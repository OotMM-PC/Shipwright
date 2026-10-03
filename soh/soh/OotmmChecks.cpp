#include "OotmmChecks.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "OotmmCheckLedger.h"
#include "OotmmDrops.h"
#include "OotmmIpc.h"
#include "OotmmItemModels.h"
#include "OotmmItemPresentation.h"
#include "OotmmSession.h"

#include <cmath>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "src/overlays/actors/ovl_En_Box/z_en_box.h"
#include "src/overlays/actors/ovl_En_Si/z_en_si.h"
#include "src/overlays/actors/ovl_En_Cow/z_en_cow.h"

extern PlayState* gPlayState;
extern void func_8084DFAC(PlayState* play, Player* player);
extern void Player_SetupActionPreserveAnimMovement(PlayState* play, Player* player, PlayerActionFunc actionFunc,
                                                   s32 flags);
extern s32 Player_SetupWaitForPutAway(PlayState* play, Player* player, AfterPutAwayFunc func);
}

namespace {

constexpr uint32_t kChestFlagMask = 0x1F;

// The chest placement bound while its give sequence runs; the slow-chest cutscene
// decision fires later in the same open and reads it.
const Ship::OotmmPlacement* sActiveChest = nullptr;
std::string sOpeningChestCheck;
uint32_t sChestOpeningFinishedFrame = 0;
bool sWasConnected = false;

const Ship::OotmmPlacement* FindChest(uint32_t scene, uint32_t flag) {
    return OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, scene, "chest", flag);
}

bool IsBossScene(uint32_t scene) {
    switch (scene) {
        case SCENE_DEKU_TREE_BOSS:
        case SCENE_DODONGOS_CAVERN_BOSS:
        case SCENE_JABU_JABU_BOSS:
        case SCENE_FOREST_TEMPLE_BOSS:
        case SCENE_FIRE_TEMPLE_BOSS:
        case SCENE_WATER_TEMPLE_BOSS:
        case SCENE_SHADOW_TEMPLE_BOSS:
        case SCENE_SPIRIT_TEMPLE_BOSS:
            return true;
        default:
            return false;
    }
}

const Ship::OotmmPlacement* FindBossContainer(uint32_t scene) {
    if (!IsBossScene(scene)) {
        return nullptr;
    }
    return OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, scene, "collectible", 0x1F);
}

const Ship::OotmmPlacement* FindCollectible(uint32_t scene, uint32_t flag) {
    return OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, scene, "collectible", flag);
}

bool IsFlaggedCollectible(const EnItem00* item) {
    return item->actor.params == ITEM00_HEART_PIECE || item->actor.params == ITEM00_SMALL_KEY;
}

const Ship::OotmmPlacement* FindToken(const Actor* actor) {
    const int bits = actor->params & 0xFF;
    if (bits == 0 || (bits & (bits - 1)) != 0) {
        return nullptr;
    }
    int key = 0;
    while (((bits >> key) & 1) == 0) {
        ++key;
    }
    key += (((actor->params + 0x100) >> 8) & 0x1F) * 8;
    return OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, "gs", std::to_string(key));
}

int CowId(const Actor* cow, const PlayState* play) {
    switch (play->sceneNum) {
        case SCENE_LINKS_HOUSE:
            return 0;
        case SCENE_GROTTOS:
            return cow->world.pos.x > 3000.0f ? 1 : 7;
        case SCENE_STABLE:
            return cow->world.pos.x < -50.0f ? 2 : 3;
        case SCENE_LON_LON_BUILDINGS:
            return cow->world.pos.z > -100.0f ? 4 : 5;
        case SCENE_IMPAS_HOUSE:
            return 6;
        case SCENE_GERUDO_VALLEY:
            return 8;
        case SCENE_JABU_JABU:
            return cow->room == 0x04 ? 9 : -1;
        default:
            return -1;
    }
}

const Ship::OotmmPlacement* FindCow(const Actor* cow, const PlayState* play) {
    const int id = CowId(cow, play);
    return id < 0 ? nullptr
                  : OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, "cow", std::to_string(id));
}

const Ship::OotmmPlacement* FindSilverRupee(int index) {
    const auto& state = OotmmSession_GetState();
    if (index < 0 || state.GetStringSetting("silverRupeeShuffle", "vanilla") == "vanilla") {
        return nullptr;
    }
    return state.FindCheck(Ship::OotmmGame::Oot, "sr", std::to_string(index));
}

void DrawPlacementItem(PlayState* play, const Ship::OotmmPlacement& placement, s16 fallback) {
    const std::string& itemId = placement.CloakItemId.empty() ? placement.ItemId : placement.CloakItemId;
    const std::string& itemName = placement.CloakItemId.empty() ? placement.ItemName : placement.CloakItemName;
    if (!OotmmItemModel_DrawById(play, itemId, itemName)) {
        GetItem_Draw(play, fallback);
    }
}

bool IsCollected(const Ship::OotmmPlacement& placement) {
    return OotmmCheckLedger_Contains(placement.CheckId) || OotmmIpc_IsCheckCompleted(placement.CheckId);
}

void Report(const Ship::OotmmPlacement& placement, bool present = false) {
    OotmmIpc_SendCheckCollected(placement.CheckId, present && !IsCollected(placement));
    OotmmCheckLedger_Record(placement.CheckId);
}

void SweepSavedBossContainers() {
    for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
        if (placement.CheckGame != Ship::OotmmGame::Oot || placement.CheckType != "collectible" ||
            !placement.CheckScene.has_value() || !placement.CheckFlag.has_value() ||
            *placement.CheckFlag != 0x1F || !IsBossScene(*placement.CheckScene)) {
            continue;
        }
        if ((gSaveContext.sceneFlags[*placement.CheckScene].collect & (1u << 0x1F)) != 0 &&
            !OotmmIpc_IsCheckCompleted(placement.CheckId)) {
            Report(placement);
        }
    }
}

void ChestOpenAction(Player* player, PlayState* play) {
    if (LinkAnimation_Update(play, &player->skelAnime)) {
        if (OotmmItemPresentation_ContinueChest(play, player, sOpeningChestCheck)) {
            sOpeningChestCheck.clear();
            return;
        }
        if (sChestOpeningFinishedFrame == 0) {
            sChestOpeningFinishedFrame = play->gameplayFrames;
        }
        if (play->gameplayFrames - sChestOpeningFinishedFrame < 120) {
            return;
        }
        sOpeningChestCheck.clear();
        func_8084DFAC(play, player);
    }
}

void ChestOpenAfterPutAway(PlayState* play, Player* player) {
    Player_SetupActionPreserveAnimMovement(play, player, ChestOpenAction, 0);
    player->stateFlags1 |= PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_CUTSCENE;
}

// Chests already opened in the save whose checks never reached the launcher —
// a crash between the flag landing and the acknowledgement — are reported here.
void SweepSavedChests() {
    for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
        if (placement.CheckGame != Ship::OotmmGame::Oot || placement.CheckType != "chest" ||
            !placement.CheckScene.has_value() || !placement.CheckFlag.has_value()) {
            continue;
        }
        const uint32_t scene = *placement.CheckScene;
        const uint32_t flag = *placement.CheckFlag;
        if (scene >= ARRAY_COUNT(gSaveContext.sceneFlags) || flag > 31) {
            continue;
        }
        uint32_t chestBits = gSaveContext.sceneFlags[scene].chest;
        if (gPlayState != nullptr && static_cast<uint32_t>(gPlayState->sceneNum) == scene) {
            chestBits |= gPlayState->actorCtx.flags.chest;
        }
        if ((chestBits & (1u << flag)) != 0 && !OotmmIpc_IsCheckCompleted(placement.CheckId)) {
            Report(placement);
        }
    }
}

} // namespace

extern "C" int OotmmChecks_BossContainerDone(PlayState* play, int vanillaDone) {
    if (play == nullptr) {
        return vanillaDone;
    }
    const auto* placement = FindBossContainer(play->sceneNum);
    return placement == nullptr ? vanillaDone : IsCollected(*placement);
}

extern "C" int OotmmChecks_BossContainerIsShuffled(PlayState* play) {
    return play != nullptr && FindBossContainer(play->sceneNum) != nullptr;
}

extern "C" int OotmmChecks_TryCollectBossContainer(Actor* actor, PlayState* play) {
    if (actor == nullptr || play == nullptr) {
        return 0;
    }
    const auto* placement = FindBossContainer(play->sceneNum);
    if (placement == nullptr) {
        return 0;
    }
    if (IsCollected(*placement)) {
        return 1;
    }
    Player* player = GET_PLAYER(play);
    if (player == nullptr || actor->xzDistToPlayer >= 30.0f || std::fabs(actor->yDistToPlayer) >= 40.0f ||
        OotmmItemPresentation_IsPending() || play->msgCtx.msgMode != MSGMODE_NONE ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || Player_InBlockingCsMode(play, player) ||
        (player->stateFlags1 &
         (PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE)) ||
        player->getItemId > GI_NONE) {
        return 0;
    }
    Report(*placement, true);
    return 1;
}

extern "C" int OotmmChecks_DrawBossContainer(Actor* actor, PlayState* play) {
    if (actor == nullptr || play == nullptr) {
        return 0;
    }
    const auto* placement = FindBossContainer(play->sceneNum);
    if (placement == nullptr) {
        return 0;
    }
    if (IsCollected(*placement)) {
        return 1;
    }
    Matrix_Push();
    if (actor->scale.x > 0.0001f) {
        const float scale = 0.35f / actor->scale.x;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    }
    DrawPlacementItem(play, *placement, GID_RUPEE_GREEN);
    Matrix_Pop();
    return 1;
}

extern "C" int OotmmChecks_DrawToken(Actor* actor, PlayState* play) {
    if (actor == nullptr || play == nullptr) {
        return 0;
    }
    const auto* placement = FindToken(actor);
    if (placement == nullptr) {
        return 0;
    }
    DrawPlacementItem(play, *placement, GID_SKULL_TOKEN_2);
    return 1;
}

extern "C" int OotmmChecks_SilverRupeeIsShuffled(int index) {
    return FindSilverRupee(index) != nullptr;
}

extern "C" int OotmmChecks_SilverRupeeTaken(int index) {
    const auto* placement = FindSilverRupee(index);
    return placement != nullptr && IsCollected(*placement);
}

extern "C" int OotmmChecks_DrawSilverRupee(Actor* actor, PlayState* play, int index) {
    const auto* placement = FindSilverRupee(index);
    if (actor == nullptr || play == nullptr || placement == nullptr) {
        return 0;
    }
    Matrix_Push();
    if (actor->scale.x > 0.0001f) {
        const float scale = 0.375f / actor->scale.x;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    }
    DrawPlacementItem(play, *placement, GID_RUPEE_GREEN);
    Matrix_Pop();
    return 1;
}

extern "C" void OotmmChecks_CollectSilverRupee(int index) {
    if (const auto* placement = FindSilverRupee(index)) {
        Report(*placement, true);
    }
}

extern "C" int OotmmChecks_DrawDivingRupee(Actor* actor, PlayState* play, int colorIdx) {
    const auto* placement = play != nullptr ? FindCollectible(play->sceneNum, colorIdx + 1) : nullptr;
    if (placement == nullptr || IsCollected(*placement)) {
        return 0;
    }
    Matrix_Push();
    if (actor->scale.x > 0.0001f) {
        const float scale = 0.25f / actor->scale.x;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    }
    DrawPlacementItem(play, *placement, GID_RUPEE_BLUE);
    Matrix_Pop();
    return 1;
}

extern "C" int OotmmChecks_CollectDivingRupee(PlayState* play, int colorIdx) {
    const auto* placement = play != nullptr ? FindCollectible(play->sceneNum, colorIdx + 1) : nullptr;
    if (placement == nullptr || IsCollected(*placement)) {
        return 0;
    }
    Flags_SetCollectible(play, colorIdx + 1);
    OotmmDrops_Report(placement->CheckId, placement->ItemId, true);
    return 1;
}

extern "C" int OotmmChecks_CollectibleDone(PlayState* play, int flag, int vanillaDone) {
    if (play == nullptr || flag < 0) {
        return vanillaDone;
    }
    const auto* placement = FindCollectible(play->sceneNum, flag);
    if (placement == nullptr) {
        return vanillaDone;
    }
    const bool collected = IsCollected(*placement);
    if (collected && !Flags_GetCollectible(play, flag)) {
        Flags_SetCollectible(play, flag);
    }
    return collected;
}

extern "C" int OotmmChecks_SpawnCollectibleDrop(PlayState* play, int flag, float x, float y, float z,
                                                  Actor** drop) {
    if (play == nullptr || flag < 0) {
        return 0;
    }
    const auto* placement = FindCollectible(play->sceneNum, flag);
    if (placement == nullptr) {
        return 0;
    }
    if (IsCollected(*placement)) {
        Flags_SetCollectible(play, flag);
        if (drop != nullptr) {
            *drop = nullptr;
        }
        return 1;
    }
    Actor* spawned = OotmmDrops_SpawnPlacement(play, x, y, z, *placement, flag);
    if (spawned == nullptr) {
        return 0;
    }
    if (drop != nullptr) {
        *drop = spawned;
    }
    return 1;
}

void OotmmChecks_OpenChestSilently(PlayState* play, Player* player) {
    Player_SetupWaitForPutAway(play, player, ChestOpenAfterPutAway);
}

void OotmmChecks_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneFlagSet>(
        [](int16_t sceneNum, int16_t flagType, int16_t flag) {
            if (flagType != FLAG_SCENE_TREASURE) {
                return;
            }
            if (const auto* placement = FindChest(sceneNum, flag)) {
                if (!OotmmCheckLedger_Contains(placement->CheckId)) {
                    Report(*placement);
                }
            }
        });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_CHEST, {
        EnBox* chest = va_arg(args, EnBox*);
        Player* player = GET_PLAYER(gPlayState);
        sActiveChest = FindChest(gPlayState->sceneNum, chest->dyna.actor.params & kChestFlagMask);
        if (sActiveChest != nullptr && player != nullptr) {
            sOpeningChestCheck = sActiveChest->CheckId;
            sChestOpeningFinishedFrame = 0;
            OotmmChecks_OpenChestSilently(gPlayState, player);
            Report(*sActiveChest, true);
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_COW, {
        EnCow* cow = va_arg(args, EnCow*);
        const auto* placement = FindCow(&cow->actor, gPlayState);
        if (placement == nullptr) {
            return;
        }
        *should = false;
        if (IsCollected(*placement)) {
            Item_Give(gPlayState, ITEM_HEART);
        } else {
            Report(*placement, true);
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_SKULL_TOKEN, {
        EnSi* token = va_arg(args, EnSi*);
        if (const auto* placement = FindToken(&token->actor)) {
            Report(*placement, true);
            SET_GS_FLAGS((token->actor.params & 0x1F00) >> 8, token->actor.params & 0xFF);
            GameInteractor_ExecuteOnFlagSet(FLAG_GS_TOKEN, token->actor.params);
            Actor_Kill(&token->actor);
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_ITEM00_DESPAWN, {
        EnItem00* item = va_arg(args, EnItem00*);
        if (IsFlaggedCollectible(item)) {
            *should = OotmmChecks_CollectibleDone(gPlayState, item->collectibleFlag, *should);
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](void* refActor) {
        Actor* actor = static_cast<Actor*>(refActor);
        if (gPlayState == nullptr || actor->id != ACTOR_EN_ITEM00 || actor->update == nullptr) {
            return;
        }
        auto* item = reinterpret_cast<EnItem00*>(actor);
        if (!IsFlaggedCollectible(item)) {
            return;
        }
        const auto* placement = FindCollectible(gPlayState->sceneNum, item->collectibleFlag);
        if (placement != nullptr && !IsCollected(*placement)) {
            OotmmDrops_Adopt(actor, *placement, item->collectibleFlag);
        }
    });

    REGISTER_VB_SHOULD(VB_PLAY_SLOW_CHEST_CS, {
        if (sActiveChest != nullptr) {
            *should = sActiveChest->Major;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(
        [](int32_t) {
            SweepSavedChests();
            SweepSavedBossContainers();
        });

    // A reconnect replays anything the launcher never acknowledged.
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([]() {
        const bool connected = OotmmIpc_IsConnected();
        if (connected && !sWasConnected) {
            SweepSavedChests();
            SweepSavedBossContainers();
        }
        sWasConnected = connected;
    });
}
