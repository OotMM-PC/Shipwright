#include "OotmmSession.h"

#include "Enhancements/enhancementTypes.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "functions.h"
#include "variables.h"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Treemouth/z_bg_treemouth.h"
}

#include <libultraship/bridge.h>
#include <spdlog/spdlog.h>

namespace {

Ship::OotmmGameState sGameState;

void KeepDekuTreeMouthState(BgTreemouth*, PlayState*) {
}

void ConfigureDekuTreeMouth(void* actorRef) {
    auto* treeMouth = static_cast<BgTreemouth*>(actorRef);
    const std::string dekuTree = sGameState.GetStringSetting("dekuTree", "open");

    bool isOpen;
    if (!LINK_IS_ADULT && dekuTree != "closed") {
        isOpen = true;
    } else if (LINK_IS_ADULT && !sGameState.WorldFlagContains("openDungeonsOot", "dekuTreeAdult")) {
        isOpen = false;
    } else {
        isOpen = Flags_GetEventChkInf(EVENTCHKINF_SHOWED_MIDO_SWORD_SHIELD);
    }

    treeMouth->unk_168 = isOpen ? 1.0f : 0.0f;
    treeMouth->actionFunc = KeepDekuTreeMouthState;
}

void ApplyMidoSpawnState(bool* should) {
    if (gPlayState == nullptr) {
        return;
    }

    if (gPlayState->sceneNum == SCENE_LOST_WOODS) {
        *should = true;
        return;
    }

    const bool midoInHouse =
        Flags_GetEventChkInf(EVENTCHKINF_SHOWED_MIDO_SWORD_SHIELD) &&
        Flags_GetEventChkInf(EVENTCHKINF_SPOKE_TO_MIDO_AFTER_DEKU_TREES_DEATH) &&
        (Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD) ||
         Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_ZELDAS_LETTER));

    if (gPlayState->sceneNum == SCENE_MIDOS_HOUSE) {
        *should = !LINK_IS_ADULT && midoInHouse;
    } else {
        *should = !midoInHouse;
    }
}

int DamageMultiplier(const std::string& value) {
    if (value == "double") {
        return 1;
    }
    if (value == "quadruple") {
        return 2;
    }
    if (value == "octuple") {
        return 3;
    }
    return value == "ohko" ? 8 : 0;
}

void ApplyEnhancements() {
    CVarSetInteger(CVAR_ENHANCEMENT("DisableCritWiggle"),
                   sGameState.GetBoolSetting("critWiggleDisable", true) ? 1 : 0);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipTowerEscape"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("FasterShadowShip"),
                   sGameState.GetBoolSetting("shadowFastBoat", false) ? 1 : 0);
    CVarSetInteger(CVAR_ENHANCEMENT("InstantScarecrow"),
                   sGameState.GetBoolSetting("freeScarecrowOot", false) ? 1 : 0);

    const std::string ageChange = sGameState.GetStringSetting("ageChange", "none");
    CVarSetInteger(CVAR_ENHANCEMENT("TimeTravel"),
                   ageChange == "oot" ? TIME_TRAVEL_OOT_MS
                                      : ageChange == "always" ? TIME_TRAVEL_ANY_MS : TIME_TRAVEL_DISABLED);
    CVarSetInteger(CVAR_CHEAT("HookshotEverything"),
                   sGameState.GetStringSetting("hookshotAnywhereOot", "off") != "off" ? 1 : 0);
    CVarSetInteger(CVAR_CHEAT("ClimbEverything"),
                   sGameState.GetStringSetting("climbMostSurfacesOot", "off") != "off" ? 1 : 0);
    CVarSetInteger(CVAR_ENHANCEMENT("DamageMult"),
                   DamageMultiplier(sGameState.GetStringSetting("damageMultiplierOot", "normal")));
    CVarSetInteger(CVAR_ENHANCEMENT("HyperEnemies"),
                   sGameState.GetBoolSetting("ootHyperEnemies", false) ? 1 : 0);
    CVarSetInteger(CVAR_CHEAT("EasyFrameAdvance"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("PauseBufferWindow"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("DpadEquips"), 1);

    const std::string csmc = sGameState.GetStringSetting("csmc", "always");
    CVarSetInteger(CVAR_ENHANCEMENT("ChestSizeAndTextureMatchContents"), csmc != "never" ? 1 : 0);
    CVarSetInteger(CVAR_ENHANCEMENT("ChestSizeDependsStoneOfAgony"), csmc == "agony" ? 1 : 0);
    CVarSetInteger(CVAR_ENHANCEMENT("MMBunnyHood"),
                   sGameState.GetBoolSetting("fastBunnyHood", true) ? BUNNY_HOOD_FAST_AND_JUMP
                                                                   : BUNNY_HOOD_VANILLA);
}

void ApplySaveFlags() {
    if (!sGameState.HasSeed()) {
        return;
    }

    Flags_SetEventChkInf(EVENTCHKINF_FIRST_SPOKE_TO_MIDO);
    Flags_SetEventChkInf(EVENTCHKINF_COMPLAINED_ABOUT_MIDO);
    Flags_SetEventChkInf(EVENTCHKINF_DEKU_TREE_OPENED_MOUTH);
    Flags_SetEventChkInf(EVENTCHKINF_MET_DEKU_TREE);
    Flags_SetInfTable(INFTABLE_GREETED_BY_SARIA);
    Flags_SetInfTable(INFTABLE_01);
    Flags_SetInfTable(INFTABLE_03);
    Flags_SetInfTable(INFTABLE_SPOKE_TO_KAEPORA_IN_LAKE_HYLIA);
    Flags_SetEventChkInf(EVENTCHKINF_SHEIK_SPAWNED_AT_MASTER_SWORD_PEDESTAL);
    Flags_SetEventChkInf(EVENTCHKINF_RENTED_HORSE_FROM_INGO);
    Flags_SetInfTable(INFTABLE_SPOKE_TO_POE_COLLECTOR_IN_RUINED_MARKET);
    Flags_SetEventChkInf(EVENTCHKINF_WATCHED_GANONS_CASTLE_COLLAPSE_CAUGHT_BY_GERUDO);
    gSaveContext.sceneFlags[SCENE_WATER_TEMPLE].swch |= 1 << 0x10;

    if (sGameState.GetStringSetting("dekuTree", "open") == "open") {
        Flags_SetEventChkInf(EVENTCHKINF_SHOWED_MIDO_SWORD_SHIELD);
    }
    if (!Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD)) {
        Flags_UnsetEventChkInf(EVENTCHKINF_SPOKE_TO_MIDO_AFTER_DEKU_TREES_DEATH);
    }
    if (sGameState.GetStringSetting("doorOfTime", "closed") == "open") {
        Flags_SetEventChkInf(EVENTCHKINF_OPENED_THE_DOOR_OF_TIME);
    }
    if (sGameState.GetStringSetting("kakarikoGate", "vanilla") == "open") {
        gSaveContext.infTable[INFTABLE_SHOWED_ZELDAS_LETTER_TO_GATE_GUARD >> 4] |=
            static_cast<uint16_t>(1 << (INFTABLE_SHOWED_ZELDAS_LETTER_TO_GATE_GUARD & 0xF));
    }

    const std::string gerudo = sGameState.GetStringSetting("gerudoFortress", "vanilla");
    if (gerudo == "open" || gerudo == "single") {
        Flags_SetEventChkInf(EVENTCHKINF_CARPENTERS_FREE(1));
        Flags_SetEventChkInf(EVENTCHKINF_CARPENTERS_FREE(2));
        Flags_SetEventChkInf(EVENTCHKINF_CARPENTERS_FREE(3));
        gSaveContext.sceneFlags[SCENE_THIEVES_HIDEOUT].swch |=
            (1 << 0x02) | (1 << 0x03) | (1 << 0x04) | (1 << 0x06) | (1 << 0x07) | (1 << 0x08) | (1 << 0x10) |
            (1 << 0x12) | (1 << 0x13);
        gSaveContext.sceneFlags[SCENE_THIEVES_HIDEOUT].collect |= (1 << 0x0A) | (1 << 0x0E) | (1 << 0x0F);
    }
    if (gerudo == "open") {
        Flags_SetEventChkInf(EVENTCHKINF_CARPENTERS_FREE(0));
        gSaveContext.sceneFlags[SCENE_THIEVES_HIDEOUT].swch |= (1 << 0x01) | (1 << 0x05) | (1 << 0x11);
        gSaveContext.sceneFlags[SCENE_THIEVES_HIDEOUT].collect |= 1 << 0x0C;
    }

    if (sGameState.GetBoolSetting("skipZelda", false)) {
        Flags_SetEventChkInf(EVENTCHKINF_OBTAINED_POCKET_EGG);
        Flags_SetEventChkInf(EVENTCHKINF_TALON_WOKEN_IN_CASTLE);
        Flags_SetEventChkInf(EVENTCHKINF_TALON_RETURNED_FROM_CASTLE);
        Flags_SetEventChkInf(EVENTCHKINF_OBTAINED_ZELDAS_LETTER);
        Flags_SetEventChkInf(EVENTCHKINF_LEARNED_ZELDAS_LULLABY);
        gSaveContext.sceneFlags[SCENE_HYRULE_CASTLE].swch |= 1 << 4;
    }

    if (sGameState.GetStringSetting("zoraKing", "vanilla") == "open") {
        Flags_SetEventChkInf(EVENTCHKINF_KING_ZORA_MOVED);
    }

    if (sGameState.GetBoolSetting("ootPreplantedBeans", false)) {
        gSaveContext.sceneFlags[SCENE_DEATH_MOUNTAIN_CRATER].swch |= 1 << 3;
        gSaveContext.sceneFlags[SCENE_DEATH_MOUNTAIN_TRAIL].swch |= 1 << 6;
        gSaveContext.sceneFlags[SCENE_DESERT_COLOSSUS].swch |= 1 << 24;
        gSaveContext.sceneFlags[SCENE_GERUDO_VALLEY].swch |= 1 << 3;
        gSaveContext.sceneFlags[SCENE_GRAVEYARD].swch |= 1 << 3;
        gSaveContext.sceneFlags[SCENE_KOKIRI_FOREST].swch |= 1 << 9;
        gSaveContext.sceneFlags[SCENE_LAKE_HYLIA].swch |= 1 << 1;
        gSaveContext.sceneFlags[SCENE_LOST_WOODS].swch |= (1 << 4) | (1 << 18);
        gSaveContext.sceneFlags[SCENE_ZORAS_RIVER].swch |= 1 << 3;
    }

    if (sGameState.WorldFlagContains("openDungeonsOot", "DC")) {
        gSaveContext.sceneFlags[SCENE_DEATH_MOUNTAIN_TRAIL].swch |= 0x10;
    }
    if (sGameState.WorldFlagContains("openDungeonsOot", "Shadow")) {
        gSaveContext.sceneFlags[SCENE_GRAVEYARD].swch |= 0xC0000000;
    }
    if (sGameState.WorldFlagContains("openDungeonsOot", "Water")) {
        gSaveContext.sceneFlags[SCENE_LAKE_HYLIA].swch |= 0x80000000;
    }
    if (sGameState.WorldFlagContains("openDungeonsOot", "BotW")) {
        Flags_SetEventChkInf(EVENTCHKINF_DRAINED_WELL_IN_KAKARIKO);
    }

    static constexpr struct {
        const char* Dungeon;
        int Flag;
    } blueWarps[] = {
        { "DT", EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP },
        { "DC", EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP },
        { "JJ", EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP },
        { "Forest", EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP },
        { "Fire", EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP },
    };
    for (const auto& dungeon : blueWarps) {
        if (sGameState.IsDungeonPreCompleted(dungeon.Dungeon)) {
            Flags_SetEventChkInf(dungeon.Flag);
        }
    }

    if (sGameState.HasWorldFlag("ganonTrials")) {
        static constexpr struct {
            const char* Trial;
            int Flag;
        } trials[] = {
            { "Light", EVENTCHKINF_COMPLETED_LIGHT_TRIAL },
            { "Forest", EVENTCHKINF_COMPLETED_FOREST_TRIAL },
            { "Fire", EVENTCHKINF_COMPLETED_FIRE_TRIAL },
            { "Water", EVENTCHKINF_COMPLETED_WATER_TRIAL },
            { "Shadow", EVENTCHKINF_COMPLETED_SHADOW_TRIAL },
            { "Spirit", EVENTCHKINF_COMPLETED_SPIRIT_TRIAL },
        };
        for (const auto& trial : trials) {
            if (!sGameState.WorldFlagContains("ganonTrials", trial.Trial)) {
                Flags_SetEventChkInf(trial.Flag);
            }
        }
    }
}

void InitializeNewSave() {
    const bool adult = sGameState.GetStringSetting("startingAge", "child") == "adult";
    gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
    gSaveContext.entranceIndex = adult ? ENTR_TEMPLE_OF_TIME_WARP_PAD : ENTR_LINKS_HOUSE_CHILD_SPAWN;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.savedSceneNum = -1;
    ApplySaveFlags();
}

} // namespace

void OotmmSession_Init() {
    if (sGameState.LoadFromEnvironment()) {
        ApplyEnhancements();
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(
            [](int32_t) { ApplySaveFlags(); });
        REGISTER_VB_SHOULD(VB_OPEN_KOKIRI_FOREST, {
            if (OotmmSession_IsActive()) {
                *should = true;
            }
        });
        REGISTER_VB_SHOULD(VB_MIDO_SPAWN, {
            if (OotmmSession_IsActive()) {
                ApplyMidoSpawnState(should);
            }
        });
        REGISTER_VB_SHOULD(VB_MIDO_CONSIDER_DEKU_TREE_DEAD, {
            if (OotmmSession_IsActive()) {
                *should = Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD);
            }
        });
        GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorInit>(ACTOR_BG_TREEMOUTH,
                                                                                     ConfigureDekuTreeMouth);
        SPDLOG_INFO("[OoTMM] Loaded seed startup state {}", sGameState.GetSeedId());
    } else if (!sGameState.GetLastError().empty()) {
        SPDLOG_ERROR("[OoTMM] {}", sGameState.GetLastError());
    }
}

extern "C" int32_t OotmmSession_IsActive(void) {
    return sGameState.IsActive() && sGameState.HasSeed() ? 1 : 0;
}

extern "C" void OotmmSession_InitializeNewSave(void) {
    if (OotmmSession_IsActive()) {
        InitializeNewSave();
    }
}

const Ship::OotmmGameState& OotmmSession_GetState() {
    return sGameState;
}

std::string OotmmSession_GetSaveSubdirectory() {
    if (!OotmmSession_IsActive()) {
        return "Save";
    }
    std::string result = "Save";
    const std::string& coordinatorSubdir = sGameState.GetBootConfig().NativeSaveSubdir;
    if (!coordinatorSubdir.empty()) {
        result += "/" + coordinatorSubdir;
    }
    return result + "/ootmm-" + sGameState.GetNativeSaveTag();
}
