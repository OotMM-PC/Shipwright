#include "OotmmChecks.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmIpc.h"
#include "OotmmItemPresentation.h"
#include "OotmmSession.h"

extern "C" {
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
bool sWasConnected = false;

const Ship::OotmmPlacement* FindChest(uint32_t scene, uint32_t flag) {
    return OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, scene, "chest", flag);
}

Ship::GameIpcItemPresentation MakePresentation(const Ship::OotmmPlacement& placement) {
    Ship::GameIpcItemPresentation presentation;
    presentation.ItemId = placement.ItemId;
    presentation.ItemName = placement.ItemName;
    presentation.ItemGame = placement.ItemGame;
    presentation.DestinationGame = placement.ItemGame;
    presentation.SourcePlayer = OotmmSession_GetState().GetPlayerId();
    presentation.RecipientPlayer = placement.OwnerPlayer;
    presentation.ViewerPlayer = presentation.SourcePlayer;
    return presentation;
}

// Chest-open continuation without the vanilla give: the open animation plays to its
// end and control returns; the queued presentation then runs the get-item raise.
void ChestOpenAction(Player* player, PlayState* play) {
    if (LinkAnimation_Update(play, &player->skelAnime)) {
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
            OotmmIpc_SendCheckCollected(placement.CheckId);
        }
    }
}

} // namespace

void OotmmChecks_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneFlagSet>(
        [](int16_t sceneNum, int16_t flagType, int16_t flag) {
            if (flagType != FLAG_SCENE_TREASURE) {
                return;
            }
            if (const auto* placement = FindChest(sceneNum, flag)) {
                OotmmIpc_SendCheckCollected(placement->CheckId);
            }
        });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_CHEST, {
        EnBox* chest = va_arg(args, EnBox*);
        Player* player = GET_PLAYER(gPlayState);
        sActiveChest = FindChest(gPlayState->sceneNum, chest->dyna.actor.params & kChestFlagMask);
        if (sActiveChest != nullptr && player != nullptr) {
            Player_SetupWaitForPutAway(gPlayState, player, ChestOpenAfterPutAway);
            OotmmItemPresentation_Queue(MakePresentation(*sActiveChest));
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_PLAY_SLOW_CHEST_CS, {
        if (sActiveChest != nullptr) {
            *should = sActiveChest->Major;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(
        [](int32_t) { SweepSavedChests(); });

    // A reconnect replays anything the launcher never acknowledged.
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([]() {
        const bool connected = OotmmIpc_IsConnected();
        if (connected && !sWasConnected) {
            SweepSavedChests();
        }
        sWasConnected = connected;
    });
}
