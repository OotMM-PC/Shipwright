#include "OotmmChecks.h"

#include "Enhancements/game-interactor/GameInteractor.h"
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
    const std::string& itemId = placement->CloakItemId.empty() ? placement->ItemId : placement->CloakItemId;
    const std::string& itemName = placement->CloakItemId.empty() ? placement->ItemName : placement->CloakItemName;
    Matrix_Push();
    if (actor->scale.x > 0.0001f) {
        const float scale = 0.35f / actor->scale.x;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    }
    if (!OotmmItemModel_DrawById(play, itemId, itemName)) {
        GetItem_Draw(play, GID_RUPEE_GREEN);
    }
    Matrix_Pop();
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
