#include "OotmmItemPresentation.h"

#include "Enhancements/custom-message/CustomMessageManager.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "OotmmDrops.h"
#include "OotmmItemModels.h"
#include "OotmmIpc.h"
#include "OotmmSession.h"
#include "OotmmSongs.h"
#include "Enhancements/audio/AudioCollection.h"
#include <libultraship/bridge/OotmmItemFanfare.h>
#include "OTRGlobals.h"
#include "ResourceManagerHelpers.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <deque>
#include <optional>
#include <string>
#include <utility>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
GetItemEntry ItemTable_Retrieve(int16_t getItemId);
s32 Player_ActionHandler_2(Player* player, PlayState* play);
void Player_Action_8084E6D4(Player* player, PlayState* play);
void Player_SetupActionPreserveAnimMovement(PlayState* play, Player* player, PlayerActionFunc actionFunc, s32 flags);
}

namespace {

constexpr uint16_t kPresentationTextId = 0xF120;
constexpr uint16_t kGrounded = 1;
constexpr int kJunkSpacingFrames = 20;
std::deque<Ship::GameIpcItemPresentation> sQueue;
std::deque<Ship::GameIpcItemPresentation> sJunk;
int sJunkCooldown = 0;
std::optional<Ship::GameIpcItemPresentation> sCurrent;
std::optional<std::pair<std::string, uint32_t>> sDeliveredCheck;

std::string GameName(const std::string& game) {
    return OotmmItemPresentation_GameName(game);
}

std::string BuildMessage(const Ship::GameIpcItemPresentation& presentation) {
    const bool isTrap = Ship::OotmmItemCatalog::IsTrap(presentation.ItemId);
    const bool isShared = presentation.ItemId.rfind("SHARED_", 0) == 0;
    const bool crossGame = !isTrap && !isShared && presentation.DestinationGame != "oot";
    const std::string itemName =
        Ship::OotmmItemCatalog::TrimGameSuffix(presentation.ItemName);
    std::string message = "You got %g" + itemName;
    if (presentation.SourcePlayer != 0 && presentation.SourcePlayer != presentation.RecipientPlayer) {
        if (presentation.ViewerPlayer == presentation.SourcePlayer) {
            message += "%w! It was sent to %bPlayer " + std::to_string(presentation.RecipientPlayer);
            message += isTrap || isShared ? std::string("%w.")
                              : "'s %r" + GameName(presentation.DestinationGame) + "%w.";
            return message;
        } else {
            message += "%w from %bPlayer " + std::to_string(presentation.SourcePlayer);
        }
    }
    message += "%w!";
    if (crossGame) {
        message += " It was sent to %r" + GameName(presentation.DestinationGame) + "%w.";
    }
    message += OotmmSession_GetState().DungeonItemText(presentation.ItemId);
    return message;
}

void DrawPresentation(PlayState* play, GetItemEntry*) {
    if (!sCurrent.has_value()) {
        return;
    }
    if (OotmmItemModel_DrawById(play, sCurrent->ItemId, sCurrent->ItemName)) {
        return;
    }
    static std::string sReported;
    if (sReported != sCurrent->ItemId) {
        sReported = sCurrent->ItemId;
        SPDLOG_WARN("OoTMM: no model for {} \"{}\"", sCurrent->ItemId, sCurrent->ItemName);
    }
    GetItem_Draw(play, GID_RUPEE_GREEN);
}

void OpenPresentationText(uint16_t* textId, bool* loadFromMessageTable) {
    if (*textId != kPresentationTextId || !sCurrent.has_value()) {
        return;
    }
    CustomMessage message(BuildMessage(*sCurrent) + (sCurrent->Background ? "\x0E\x50" : ""), TEXTBOX_TYPE_BLUE);
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

GetItemEntry PresentationEntry() {
    GetItemEntry entry = ItemTable_Retrieve(GI_RUPEE_GREEN);
    entry.textId = kPresentationTextId;
    entry.getItemFrom = ITEM_FROM_OOTMM_PRESENTATION;
    entry.drawFunc = DrawPresentation;
    return entry;
}

void PlayItemFanfare(const std::string& itemId, bool isShort);

void PumpJunk() {
    if (sJunkCooldown > 0) {
        sJunkCooldown--;
    }
    if (sJunk.empty() || sJunkCooldown > 0 || gPlayState == nullptr ||
        gPlayState->transitionTrigger != TRANS_TRIGGER_OFF || gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return;
    }
    const auto& junk = sJunk.front();
    if (OotmmDrops_ShowOverhead(gPlayState, junk.ItemId, junk.ItemName)) {
        PlayItemFanfare(junk.ItemId, true);
        sJunk.pop_front();
        sJunkCooldown = kJunkSpacingFrames;
    }
}

void PumpPresentation() {
    PumpJunk();
    if (gPlayState != nullptr && sDeliveredCheck.has_value() &&
        sDeliveredCheck->second != gPlayState->gameplayFrames) {
        const Player* player = GET_PLAYER(gPlayState);
        if (Message_GetState(&gPlayState->msgCtx) == TEXT_STATE_NONE &&
            !(player->stateFlags1 & (PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE))) {
            OotmmIpc_CompleteCheckPresentation(sDeliveredCheck->first);
            sDeliveredCheck.reset();
        }
    }
    if (gPlayState == nullptr || sQueue.empty()) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return;
    }
    if (gPlayState->msgCtx.msgMode != MSGMODE_NONE || gPlayState->pauseCtx.state != 0 ||
        gPlayState->pauseCtx.debugState != 0 ||
        gPlayState->gameOverCtx.state != GAMEOVER_INACTIVE ||
        gPlayState->transitionTrigger != TRANS_TRIGGER_OFF ||
        gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return;
    }

    if (Player_InBlockingCsMode(gPlayState, player) ||
        (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM |
                                PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE)) ||
        OotmmItemPresentation_PlayerReceiving(player)) {
        return;
    }

    GetItemEntry entry = PresentationEntry();
    sCurrent = sQueue.front();

    if (!sCurrent->Background && (player->actor.bgCheckFlags & kGrounded) &&
        !(player->stateFlags1 & PLAYER_STATE1_IN_WATER)) {
        player->getItemEntry = entry;
        player->getItemId = entry.getItemId;
        player->interactRangeActor = &player->actor;
        player->getItemDirection = player->actor.shape.rot.y;
        if (Player_ActionHandler_2(player, gPlayState)) {
            if (!sCurrent->CheckKey.empty()) {
                sDeliveredCheck = std::make_pair(sCurrent->CheckKey, gPlayState->gameplayFrames);
            }
            sQueue.pop_front();
        } else {
            sCurrent.reset();
            player->getItemId = GI_NONE;
        }
        return;
    }

    sQueue.pop_front();
    if (sCurrent->Background) {
        OotmmDrops_ShowOverhead(gPlayState, sCurrent->ItemId, sCurrent->ItemName);
    }
    OotmmItemPresentation_PlayFanfare(1);
    Message_StartTextbox(gPlayState, kPresentationTextId, nullptr);
    if (!sCurrent->CheckKey.empty()) {
        sDeliveredCheck = std::make_pair(sCurrent->CheckKey, gPlayState->gameplayFrames);
    }
}

void PlaySfx(uint16_t sfx) {
    Audio_PlaySoundGeneral(sfx, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultReverb);
}

int32_t SongFanfare(int32_t song) {
    const int32_t seq = OotmmSongs_FanfareSeqId(song);
    return seq > 0 ? seq : NA_BGM_ITEM_GET | 0x900;
}

int32_t ForeignFanfare(const char* name, const char* fontPath) {
    const int32_t seq = AudioCollection::Instance->GetSequenceNumByName(name);
    SoundFont* font = ResourceMgr_LoadAudioSoundFontByName(fontPath);
    SequenceData* data = ResourceMgr_LoadSeqPtrByName((std::string("custom/music/") + name + "_fanfare").c_str());
    if (seq <= 0 || font == nullptr || data == nullptr) {
        return NA_BGM_ITEM_GET | 0x900;
    }
    data->fonts[0] = static_cast<uint8_t>(font->fntIndex);
    data->numFonts = 1;
    return seq;
}

int32_t FanfareSeq(Ship::OotmmFanfare fanfare) {
    using F = Ship::OotmmFanfare;
    switch (fanfare) {
        case F::HeartContainer:
            return NA_BGM_HEART_GET | 0x900;
        case F::HeartPiece:
            return NA_BGM_SMALL_ITEM_GET;
        case F::Song:
            return NA_BGM_OCA_FAIRY_GET;
        case F::SongTime:
            return NA_BGM_OCA_TIME;
        case F::SongEpona:
            return NA_BGM_OCA_EPONA;
        case F::SongSun:
            return NA_BGM_OCA_SUNS;
        case F::SongStorms:
            return NA_BGM_OCA_STORM;
        case F::Medallion:
            return NA_BGM_MEDALLION_GET;
        case F::Stone:
            return NA_BGM_SPIRITUAL_STONE;
        case F::SongZelda:
            return NA_BGM_OCA_ZELDA;
        case F::SongSaria:
            return NA_BGM_OCA_SARIA;
        case F::SongLight:
            return NA_BGM_OCA_LIGHT;
        case F::SongForest:
            return NA_BGM_OCA_MINUET;
        case F::SongFire:
            return NA_BGM_OCA_BOLERO;
        case F::SongWater:
            return NA_BGM_OCA_SERENADE;
        case F::SongShadow:
            return NA_BGM_OCA_NOCTURNE;
        case F::SongSpirit:
            return NA_BGM_OCA_REQUIEM;
        case F::SongSoaring:
            return SongFanfare(OOTMM_SONG_SOARING);
        case F::SongHealing:
            return SongFanfare(OOTMM_SONG_HEALING);
        case F::SongAwakening:
            return SongFanfare(OOTMM_SONG_AWAKENING);
        case F::SongGoronHalf:
            return SongFanfare(OOTMM_SONG_GORON_HALF);
        case F::SongGoron:
            return SongFanfare(OOTMM_SONG_GORON);
        case F::SongZora:
            return SongFanfare(OOTMM_SONG_ZORA);
        case F::SongEmptiness:
            return SongFanfare(OOTMM_SONG_ELEGY);
        case F::SongOrder:
            return SongFanfare(OOTMM_SONG_ORDER);
        case F::Mask:
            return ForeignFanfare("GetMask", "custom/fonts/MmGetMask");
        default:
            return NA_BGM_ITEM_GET | 0x900;
    }
}

void PlayItemFanfare(const std::string& itemId, bool isShort) {
    const auto& inventory = OotmmIpc_GetInventory();
    const auto fanfare = Ship::OotmmItemFanfare_Resolve(
        itemId, isShort,
        [&inventory](std::string_view id) { return inventory.Count(std::string(id)); },
        static_cast<uint32_t>(OotmmSession_GetState().GetIntSetting("strayFairyRewardCount", 15)));
    switch (fanfare) {
        case Ship::OotmmFanfare::None:
            break;
        case Ship::OotmmFanfare::SfxRupee:
            PlaySfx(NA_SE_SY_GET_RUPY);
            break;
        case Ship::OotmmFanfare::SfxMinor:
            PlaySfx(NA_SE_SY_GET_ITEM);
            break;
        case Ship::OotmmFanfare::SfxMinorQuick:
            PlaySfx(NA_SE_SY_GET_BOXITEM);
            break;
        default:
            Audio_PlayFanfare(static_cast<u16>(FanfareSeq(fanfare)));
            break;
    }
}

} // namespace

extern "C" int OotmmItemPresentation_PlayFanfare(int isShort) {
    if (!sCurrent.has_value()) {
        return 0;
    }
    PlayItemFanfare(sCurrent->ItemId, isShort != 0);
    return 1;
}

void OotmmItemPresentation_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenPresentationText);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(PumpPresentation);
}

std::string OotmmItemPresentation_GameName(const std::string& game) {
    return game == "oot" ? "Ocarina of Time" : "Majora's Mask";
}

extern "C" int OotmmItemPresentation_IsPending(void) {
    return sQueue.empty() ? 0 : 1;
}

void OotmmItemPresentation_Queue(Ship::GameIpcItemPresentation presentation) {
    if (presentation.Background) {
        if (Ship::OotmmItemCatalog::IsNothing(presentation.ItemId)) {
            return;
        }
        if (Ship::OotmmItemCatalog::IsFast(presentation.ItemId)) {
            sJunk.emplace_back(std::move(presentation));
            return;
        }
        if (presentation.Quiet) {
            return;
        }
    }
    sQueue.emplace_back(std::move(presentation));
}

bool OotmmItemPresentation_PlayerReceiving(const Player* player) {
    return player->getItemId > GI_NONE && player->getItemId < GI_MAX;
}

bool OotmmItemPresentation_IsDelivering(const std::string& checkId) {
    return sDeliveredCheck.has_value() && sDeliveredCheck->first == "oot:" + checkId;
}

bool OotmmItemPresentation_ContinueChest(PlayState* play, Player* player, const std::string& checkId) {
    if (play == nullptr || player == nullptr || checkId.empty() || sDeliveredCheck.has_value()) {
        return false;
    }
    const std::string checkKey = "oot:" + checkId;
    const auto queued = std::find_if(sQueue.begin(), sQueue.end(), [&](const auto& presentation) {
        return presentation.CheckKey == checkKey;
    });
    if (queued == sQueue.end()) {
        return false;
    }
    sCurrent = std::move(*queued);
    sQueue.erase(queued);
    const GetItemEntry entry = PresentationEntry();
    player->getItemEntry = entry;
    player->getItemId = entry.getItemId;
    player->interactRangeActor = &player->actor;
    Player_SetupActionPreserveAnimMovement(play, player, Player_Action_8084E6D4, 0);
    player->stateFlags1 |= PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_IN_CUTSCENE;
    sDeliveredCheck = std::make_pair(checkKey, play->gameplayFrames);
    return true;
}
