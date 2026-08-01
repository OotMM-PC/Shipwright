#include "OotmmTriforce.h"

#include "OotmmItemApply.h"
#include "OotmmIpc.h"
#include "OotmmSession.h"
#include "SaveManager.h"

#include <libultraship/bridge/OotmmItemGrant.h>

#include <string>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
}

namespace {

const std::string kWinKey = "@triforce_win";

std::string Goal() {
    return OotmmSession_GetState().GetStringSetting("goal", "both");
}

bool IsQuest() {
    return Goal() == "triforce3";
}

bool IsPiece(const std::string& itemId) {
    if (itemId.ends_with("_FULL")) {
        return false;
    }
    const auto ops = Ship::OotmmItemGrant::Resolve(itemId, 1);
    return !ops.empty() && ops.front().Kind == Ship::OotmmGrantKind::Triforce;
}

int PieceCount() {
    int total = 0;
    for (const auto& [itemId, count] : OotmmIpc_GetInventory().GetValues()) {
        if (count > 0 && IsPiece(itemId)) {
            total += static_cast<int>(count);
        }
    }
    return total;
}

uint8_t sSafeFrames = 0;

bool OnSafeFrame(PlayState* play) {
    if (play == nullptr || gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.minigameState == 1) {
        return false;
    }
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF) {
        return false;
    }
    if (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE || Player_InCsMode(play)) {
        return false;
    }
    Player* player = GET_PLAYER(play);
    if (player == nullptr) {
        return false;
    }
    const u32 busy = PLAYER_STATE1_LOADING | PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM |
                     PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE |
                     PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_FLOOR_DISABLED;
    return (player->stateFlags1 & busy) == 0;
}

bool SafeToWarp(PlayState* play) {
    if (!OnSafeFrame(play)) {
        sSafeFrames = 0;
        return false;
    }
    if (sSafeFrames < 4) {
        sSafeFrames++;
    }
    return sSafeFrames >= 4;
}

void CreditWarp(PlayState* play) {
    GET_PLAYER(play)->stateFlags1 |= PLAYER_STATE1_INPUT_DISABLED;
    SaveManager::Instance->SaveFile(gSaveContext.fileNum);
    gSaveContext.nextCutsceneIndex = 0xFFF2;
    play->nextEntranceIndex = ENTR_CHAMBER_OF_THE_SAGES_0;
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
}

} // namespace

extern "C" int OotmmTriforce_Active(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const std::string goal = Goal();
    return goal == "triforce" || goal == "triforce3";
}

extern "C" int OotmmTriforce_Count(void) {
    return OotmmTriforce_Active() ? PieceCount() : 0;
}

extern "C" int OotmmTriforce_Goal(void) {
    if (!OotmmTriforce_Active()) {
        return 0;
    }
    if (IsQuest()) {
        return 3;
    }
    return OotmmSession_GetState().GetIntSetting("triforceGoal", 20);
}

extern "C" int OotmmTriforce_DisplayMax(void) {
    if (!OotmmTriforce_Active()) {
        return 0;
    }
    if (IsQuest() || !OotmmTriforce_HasWon()) {
        return OotmmTriforce_Goal();
    }
    return OotmmSession_GetState().GetIntSetting("triforcePieces", OotmmTriforce_Goal());
}

extern "C" int OotmmTriforce_HasWon(void) {
    return OotmmTriforce_Active() && OotmmItemApply_LedgerGet(kWinKey) != 0;
}

extern "C" void OotmmTriforce_Update(void) {
    if (!OotmmTriforce_Active() || OotmmTriforce_HasWon()) {
        return;
    }
    const int goal = OotmmTriforce_Goal();
    if (goal <= 0 || PieceCount() < goal || !SafeToWarp(gPlayState)) {
        return;
    }
    OotmmItemApply_LedgerSet(kWinKey, 1);
    CreditWarp(gPlayState);
}
