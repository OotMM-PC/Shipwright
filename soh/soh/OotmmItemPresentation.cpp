#include "OotmmItemPresentation.h"

#include "Enhancements/custom-message/CustomMessageManager.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "OotmmItemModels.h"
#include "OotmmIpc.h"
#include "OTRGlobals.h"

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
std::deque<Ship::GameIpcItemPresentation> sQueue;
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
    CustomMessage message(BuildMessage(*sCurrent), TEXTBOX_TYPE_BLUE);
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

void PumpPresentation() {
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
        player->getItemId > GI_NONE) {
        return;
    }

    GetItemEntry entry = PresentationEntry();
    sCurrent = sQueue.front();

    if ((player->actor.bgCheckFlags & kGrounded) && !(player->stateFlags1 & PLAYER_STATE1_IN_WATER)) {
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
    Audio_PlaySoundGeneral(NA_SE_SY_GET_ITEM, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    Message_StartTextbox(gPlayState, kPresentationTextId, nullptr);
    if (!sCurrent->CheckKey.empty()) {
        sDeliveredCheck = std::make_pair(sCurrent->CheckKey, gPlayState->gameplayFrames);
    }
}

} // namespace

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
    sQueue.emplace_back(std::move(presentation));
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
