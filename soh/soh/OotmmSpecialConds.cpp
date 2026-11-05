#include "OotmmSpecialConds.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "OotmmIpc.h"
#include "OotmmItemPresentation.h"
#include "OotmmSession.h"
#include "OotmmSouls.h"

#include <string>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
}

namespace {

bool SpecialCondMet(const char* name) {
    return OotmmSession_GetState().IsSpecialCondMet(name, OotmmIpc_GetInventory());
}

std::string Setting(const char* name, const char* fallback) {
    return OotmmSession_GetState().GetStringSetting(name, fallback);
}

bool RainbowBridgeOpen() {
    const std::string mode = Setting("rainbowBridge", "medallions");
    if (mode == "custom") {
        return SpecialCondMet("BRIDGE");
    }
    if (mode == "vanilla") {
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW) && CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT) &&
               INV_CONTENT(ITEM_ARROW_LIGHT) == ITEM_ARROW_LIGHT;
    }
    if (mode == "medallions") {
        return CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST) && CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE) &&
               CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER) && CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT) &&
               CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW) && CHECK_QUEST_ITEM(QUEST_MEDALLION_LIGHT);
    }
    return true;
}

bool LacsReady() {
    if (!LINK_IS_ADULT || Flags_GetEventChkInf(EVENTCHKINF_RETURNED_TO_TEMPLE_OF_TIME_WITH_ALL_MEDALLIONS) ||
        OotmmSouls_Withheld("NPC_SHEIK") || OotmmSouls_Withheld("NPC_ZELDA")) {
        return false;
    }
    if (Setting("lacs", "vanilla") == "custom") {
        return SpecialCondMet("LACS");
    }
    return CHECK_QUEST_ITEM(QUEST_MEDALLION_SPIRIT) && CHECK_QUEST_ITEM(QUEST_MEDALLION_SHADOW);
}

bool SafeFrame(PlayState* play) {
    if (play == nullptr || gSaveContext.gameMode != GAMEMODE_NORMAL || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || Message_GetState(&play->msgCtx) != TEXT_STATE_NONE ||
        Player_InCsMode(play)) {
        return false;
    }
    Player* player = GET_PLAYER(play);
    return player != nullptr && (player->stateFlags1 & (PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_CUTSCENE |
                                                        PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD)) == 0;
}

void GiveLightArrowsInTemple() {
    if (gPlayState == nullptr || gPlayState->sceneNum != SCENE_TEMPLE_OF_TIME || !SafeFrame(gPlayState) ||
        !LacsReady()) {
        return;
    }
    Flags_SetEventChkInf(EVENTCHKINF_RETURNED_TO_TEMPLE_OF_TIME_WITH_ALL_MEDALLIONS);
    if (GameInteractor_Should(VB_GIVE_ITEM_LIGHT_ARROW, true)) {
        Item_Give(gPlayState, ITEM_ARROW_LIGHT);
    }
}

void RunStoryTriggers() {
    if (gPlayState == nullptr || !SafeFrame(gPlayState)) {
        return;
    }
    if (!OotmmSouls_Withheld("NPC_SHEIK") && gSaveContext.entranceIndex == ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE &&
        !Flags_GetEventChkInf(EVENTCHKINF_LEARNED_REQUIEM_OF_SPIRIT)) {
        Flags_SetEventChkInf(EVENTCHKINF_LEARNED_REQUIEM_OF_SPIRIT);
        if (GameInteractor_Should(VB_GIVE_ITEM_SONG, true, ITEM_SONG_REQUIEM)) {
            Item_Give(gPlayState, ITEM_SONG_REQUIEM);
        }
        return;
    }
    if (!OotmmSouls_Withheld("NPC_SHEIK") && gPlayState->sceneNum == SCENE_KAKARIKO_VILLAGE && LINK_IS_ADULT &&
        CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST) && CHECK_QUEST_ITEM(QUEST_MEDALLION_FIRE) &&
        CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER) && !Flags_GetEventChkInf(EVENTCHKINF_LEARNED_NOCTURNE_OF_SHADOW)) {
        Flags_SetEventChkInf(EVENTCHKINF_LEARNED_NOCTURNE_OF_SHADOW);
        Flags_SetEventChkInf(EVENTCHKINF_BONGO_BONGO_ESCAPED_FROM_WELL);
        if (GameInteractor_Should(VB_GIVE_ITEM_SONG, true, ITEM_SONG_NOCTURNE)) {
            Item_Give(gPlayState, ITEM_SONG_NOCTURNE);
        }
        return;
    }
    if (!OotmmSouls_Withheld("NPC_SARIA") && gSaveContext.entranceIndex == ENTR_LOST_WOODS_BRIDGE_EAST_EXIT &&
        !Flags_GetEventChkInf(EVENTCHKINF_SPOKE_TO_SARIA_ON_BRIDGE)) {
        Flags_SetEventChkInf(EVENTCHKINF_SPOKE_TO_SARIA_ON_BRIDGE);
        if (GameInteractor_Should(VB_GIVE_ITEM_FAIRY_OCARINA, true)) {
            Item_Give(gPlayState, ITEM_OCARINA_FAIRY);
        }
    }
}

void GrantGanonBossKey() {
    if (gPlayState == nullptr || Setting("ganonBossKey", "removed") != "custom" ||
        CHECK_DUNGEON_ITEM(DUNGEON_KEY_BOSS, SCENE_GANONS_TOWER) || !SpecialCondMet("GANON_BK")) {
        return;
    }
    gSaveContext.inventory.dungeonItems[SCENE_GANONS_TOWER] |= gBitFlags[DUNGEON_KEY_BOSS];
    Ship::GameIpcItemPresentation presentation;
    presentation.ItemId = "OOT_BOSS_KEY_GANON";
    presentation.ItemName = "Boss Key (Ganon's Castle)";
    presentation.ItemGame = "oot";
    presentation.DestinationGame = "oot";
    OotmmItemPresentation_Queue(presentation);
}

} // namespace

void OotmmSpecialConds_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([]() {
        if (OotmmSession_IsActive()) {
            GrantGanonBossKey();
            GiveLightArrowsInTemple();
            RunStoryTriggers();
        }
    });
    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_NOCTURNE_OF_SHADOW, {
        if (OotmmSession_IsActive()) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_RAINBOW_BRIDGE, {
        if (OotmmSession_IsActive()) {
            *should = RainbowBridgeOpen();
        }
    });
    REGISTER_VB_SHOULD(VB_PLAY_RAINBOW_BRIDGE_CS, {
        if (OotmmSession_IsActive()) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_LIGHT_ARROWS, {
        if (OotmmSession_IsActive()) {
            *should = gEntranceTable[gSaveContext.entranceIndex].scene == SCENE_TEMPLE_OF_TIME && LacsReady();
        }
    });
}
