#include "OotmmDeathLink.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmIpc.h"

#include <spdlog/spdlog.h>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
}

namespace {

constexpr int kRemoteAttributionFrames = 30;

bool sPendingRemote = false;
int sRemoteWindow = 0;
bool sWasDead = false;

constexpr u32 kBusyStates = PLAYER_STATE1_TALKING | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_ON_HORSE |
                             PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE;

bool CanApply(Player* player) {
    return gSaveContext.health > 0 && gSaveContext.gameMode == GAMEMODE_NORMAL && gPlayState->pauseCtx.state == 0 &&
           gPlayState->transitionTrigger == TRANS_TRIGGER_OFF && gPlayState->transitionMode == TRANS_MODE_OFF &&
           gPlayState->msgCtx.msgMode == MSGMODE_NONE && gPlayState->csCtx.state == CS_STATE_IDLE &&
           !Player_InCsMode(gPlayState) && !(player->stateFlags1 & kBusyStates);
}

void Update() {
    if (gPlayState == nullptr) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return;
    }
    const bool dead = (player->stateFlags1 & PLAYER_STATE1_DEAD) != 0;
    if (dead && !sWasDead) {
        if (gPlayState->gameOverCtx.state >= GAMEOVER_REVIVE_START &&
            gPlayState->gameOverCtx.state <= GAMEOVER_REVIVE_FADE_OUT) {
            SPDLOG_INFO("[OoTMM death link] fairy revival; not reported");
        } else if (sRemoteWindow == 0 && !sPendingRemote) {
            SPDLOG_INFO("[OoTMM death link] reported local death (sent={})", OotmmIpc_SendDeathLink());
        } else {
            SPDLOG_INFO("[OoTMM death link] death came from the link; not reported");
        }
        sRemoteWindow = 0;
    }
    sWasDead = dead;
    if (sRemoteWindow > 0) {
        sRemoteWindow--;
    }
    if (!sPendingRemote) {
        return;
    }
    if (dead) {
        sPendingRemote = false;
    } else if (CanApply(player)) {
        sPendingRemote = false;
        sRemoteWindow = kRemoteAttributionFrames;
        gSaveContext.health = 0;
        SPDLOG_INFO("[OoTMM death link] applied remote death");
    }
}

}

void OotmmDeathLink_Receive() {
    SPDLOG_INFO("[OoTMM death link] received remote death");
    sPendingRemote = true;
}

void OotmmDeathLink_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(Update);
}
