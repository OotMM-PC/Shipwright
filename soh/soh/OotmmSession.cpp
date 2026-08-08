#include "OotmmSession.h"
#include "OotmmChecks.h"
#include "OotmmCustomItems.h"
#include "OotmmItemApply.h"
#include "OotmmItemPresentation.h"
#include "OotmmItemProbe.h"
#include "OotmmScales.h"
#include "OotmmPresence.h"

#include "Enhancements/enhancementTypes.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "OotmmIpc.h"
#include "SaveManager.h"
#include "functions.h"
#include "variables.h"

extern "C" {
#include "src/overlays/actors/ovl_Bg_Treemouth/z_bg_treemouth.h"
void Save_SaveFile(void);
uint32_t Save_Exist(int fileNum);
void Sram_InitNewSave(void);
}

#include <libultraship/bridge.h>
#include <ship/Context.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

namespace {

Ship::OotmmGameState sGameState;
bool sBootedIntoGame = false;
bool sBootEntranceApplied = false;
bool sBootRespawnPending = false;
std::optional<uint32_t> sLoadSpawnDestination;
std::optional<uint32_t> sLoadSpawnCrossGameSource;
bool sLoadSpawnHandoffReady = false;
bool sPlayerExitPending = false;
bool sCrossGamePending = false;
bool sCrossGameAccepted = false;
uint32_t sCrossGameWaitFrames = 0;
std::optional<uint32_t> sPendingExtendedSource;
std::optional<uint32_t> sLastResolvedEntrance;
uint16_t sGrottoExitSource = 0;
uint16_t sGrottoReturnEntrance = 0;
// respawnFlag value that restores respawn[RESPAWN_MODE_RETURN].
constexpr int32_t kRespawnGrottoPopOut = 2;

std::filesystem::path ActiveFileMarkerPath() {
    // Beside the session state, where MM can read it too.
    return std::filesystem::path(sGameState.GetBootConfig().StatePath + ".last-file");
}

void DeleteForeignSave(int32_t fileNum) {
    const auto& boot = sGameState.GetBootConfig();
    std::error_code ec;
    if (!boot.MmSaveDir.empty()) {
        const std::filesystem::path dir =
            std::filesystem::path(boot.MmSaveDir) / ("ootmm-" + sGameState.GetNativeSaveTag());
        const std::string stem = "file" + std::to_string(fileNum + 1);
        std::filesystem::remove(dir / (stem + ".json"), ec);
        std::filesystem::remove(dir / (stem + "backup.json"), ec);
    }
    // A surviving ledger would suppress every re-grant onto the file that replaces this one.
    if (!boot.StatePath.empty()) {
        std::filesystem::remove(boot.StatePath + ".applied." + std::to_string(fileNum), ec);
    }
}

void RecordActiveSaveFile(int32_t fileNum) {
    if (fileNum < 0 || fileNum >= SaveManager::MaxFiles) {
        return;
    }
    const std::filesystem::path path = ActiveFileMarkerPath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    out << fileNum;
}

std::optional<int32_t> RecordedActiveSaveFile() {
    std::ifstream in(ActiveFileMarkerPath());
    int32_t fileNum = -1;
    if (!(in >> fileNum) || fileNum < 0 || fileNum >= SaveManager::MaxFiles) {
        return std::nullopt;
    }
    return fileNum;
}

struct OotGrottoExit {
    uint16_t Entrance;
    uint8_t Room;
    int16_t Position[3];
};

struct OotGrottoLoad {
    uint16_t OotmmEntryId;
    uint16_t SohEntrance;
    uint8_t Content;
    uint16_t ExitId;
};

constexpr uint32_t kOotGrottoExitBase = 0x1100;
constexpr size_t kOotGrottoCount = 33;

#include "OotmmEntranceTables.inc"

void ApplyGrottoExit(const OotGrottoExit& exit) {
    RespawnData* respawn = &gSaveContext.respawn[RESPAWN_MODE_RETURN];
    respawn->pos.x = static_cast<float>(exit.Position[0]);
    respawn->pos.y = static_cast<float>(exit.Position[1]);
    respawn->pos.z = static_cast<float>(exit.Position[2]);
    respawn->yaw = 0;
    respawn->playerParams = 0x04FF;
    respawn->entranceIndex = exit.Entrance;
    respawn->roomIndex = exit.Room;
    respawn->data = 0;
    respawn->tempSwchFlags = 0;
    respawn->tempCollectFlags = 0;
    gSaveContext.respawn[RESPAWN_MODE_DOWN] = *respawn;
    gSaveContext.respawnFlag = kRespawnGrottoPopOut;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
    sGrottoReturnEntrance = exit.Entrance;
}

uint32_t TranslateGrottoEntry(uint16_t entrance, int16_t scene, int8_t data) {
    switch (entrance) {
        case ENTR_GROTTOS_0:
            switch (data & 0x1F) {
                case 0x0C:
                    return 0x1000;
                case 0x14:
                    return 0x1001;
                case 0x08:
                    return 0x1002;
                case 0x17:
                    return 0x1003;
                case 0x1A:
                    return 0x1004;
                case 0x09:
                    return 0x1005;
                case 0x02:
                    return 0x1006;
                case 0x03:
                    return 0x1007;
                case 0x00:
                    return 0x1008;
                default:
                    return 0;
            }
        case ENTR_FAIRYS_FOUNTAIN_0:
            switch (scene) {
                case SCENE_SACRED_FOREST_MEADOW:
                    return 0x1009;
                case SCENE_HYRULE_FIELD:
                    return 0x100A;
                case SCENE_ZORAS_RIVER:
                    return 0x100B;
                case SCENE_ZORAS_DOMAIN:
                    return 0x100C;
                case SCENE_GERUDOS_FORTRESS:
                    return 0x100D;
                default:
                    return 0;
            }
        case ENTR_GROTTOS_10:
            switch (scene) {
                case SCENE_SACRED_FOREST_MEADOW:
                    return 0x100E;
                case SCENE_ZORAS_RIVER:
                    return 0x100F;
                case SCENE_GERUDO_VALLEY:
                    return 0x1010;
                case SCENE_DESERT_COLOSSUS:
                    return 0x1011;
                default:
                    return 0;
            }
        case ENTR_GROTTOS_4:
            switch (scene) {
                case SCENE_LON_LON_RANCH:
                    return 0x1012;
                case SCENE_GORON_CITY:
                    return 0x1013;
                case SCENE_DEATH_MOUNTAIN_CRATER:
                    return 0x1014;
                case SCENE_LAKE_HYLIA:
                    return 0x1015;
                default:
                    return 0;
            }
        default:
            for (const auto& load : kOotGrottoLoads) {
                if (load.SohEntrance == entrance) {
                    return load.OotmmEntryId;
                }
            }
            return 0;
    }
}

std::optional<uint16_t> ResolveOotEntrance(uint32_t entrance) {
    switch (entrance) {
        case 0x0F20:
            return ENTR_TEMPLE_OF_TIME_WARP_PAD;
        case 0x00BB:
            return ENTR_LINKS_HOUSE_CHILD_SPAWN;
        case 0x0F00:
            return ENTR_CASTLE_GROUNDS_GREAT_FAIRY_EXIT;
        case 0x0F01:
            return ENTR_LOST_WOODS_SOUTH_EXIT;
        default:
            break;
    }

    if (entrance >= kOotGrottoExitBase && entrance < kOotGrottoExitBase + kOotGrottoCount) {
        const auto& exit = kOotGrottoExits[entrance - kOotGrottoExitBase];
        ApplyGrottoExit(exit);
        return exit.Entrance;
    }
    for (const auto& load : kOotGrottoLoads) {
        if (load.OotmmEntryId != entrance) {
            continue;
        }
        const auto& exit = kOotGrottoExits[load.ExitId - kOotGrottoExitBase];
        RespawnData* respawn = &gSaveContext.respawn[RESPAWN_MODE_RETURN];
        respawn->pos.x = static_cast<float>(exit.Position[0]);
        respawn->pos.y = static_cast<float>(exit.Position[1]);
        respawn->pos.z = static_cast<float>(exit.Position[2]);
        respawn->yaw = 0;
        respawn->playerParams = 0x04FF;
        respawn->entranceIndex = exit.Entrance;
        respawn->roomIndex = exit.Room;
        respawn->data = static_cast<int8_t>(load.Content);
        respawn->tempSwchFlags = 0;
        respawn->tempCollectFlags = 0;
        sGrottoExitSource = load.ExitId;
        return load.SohEntrance;
    }
    return entrance < ENTR_MAX ? std::optional<uint16_t>(static_cast<uint16_t>(entrance)) : std::nullopt;
}

uint32_t CurrentGrottoExitSource() {
    if (sGrottoExitSource != 0) {
        return sGrottoExitSource;
    }
    const int16_t returnEntrance = gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex;
    if (returnEntrance < 0 || returnEntrance >= ENTR_MAX) {
        return 0;
    }
    const uint32_t entry = TranslateGrottoEntry(
        static_cast<uint16_t>(gSaveContext.entranceIndex), gEntranceTable[returnEntrance].scene,
        gSaveContext.respawn[RESPAWN_MODE_RETURN].data);
    for (const auto& load : kOotGrottoLoads) {
        if (load.OotmmEntryId == entry) {
            return load.ExitId;
        }
    }
    return 0;
}

bool IsResolvedReloadTarget(uint32_t entrance) {
    for (int i = 0; i < RESPAWN_MODE_MAX; ++i) {
        if (entrance == static_cast<uint16_t>(gSaveContext.respawn[i].entranceIndex)) {
            return true;
        }
    }
    return entrance == static_cast<uint16_t>(gSaveContext.entranceIndex);
}

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

const char* MasterQuestMember(int32_t sceneNum) {
    switch (sceneNum) {
        case SCENE_DEKU_TREE:
            return "DT";
        case SCENE_DODONGOS_CAVERN:
            return "DC";
        case SCENE_JABU_JABU:
            return "JJ";
        case SCENE_FOREST_TEMPLE:
            return "Forest";
        case SCENE_FIRE_TEMPLE:
            return "Fire";
        case SCENE_WATER_TEMPLE:
            return "Water";
        case SCENE_SPIRIT_TEMPLE:
            return "Spirit";
        case SCENE_SHADOW_TEMPLE:
            return "Shadow";
        case SCENE_BOTTOM_OF_THE_WELL:
            return "BotW";
        case SCENE_ICE_CAVERN:
            return "IC";
        case SCENE_GERUDO_TRAINING_GROUND:
            return "GTG";
        case SCENE_INSIDE_GANONS_CASTLE:
            return "Ganon";
        default:
            return nullptr;
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
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.Intro"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.Entrances"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.Story"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.LearnSong"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.BossIntro"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.QuickBossDeaths"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipCutscene.OnePoint"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipOwlInteractions"), 1);
    CVarSetInteger(CVAR_ENHANCEMENT("TimeSavers.SkipMiscInteractions"), 1);
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

void ApplyBootEntrance() {
    if (sBootEntranceApplied) {
        return;
    }

    const auto& boot = sGameState.GetBootConfig();
    if (boot.OotAge.has_value() && *boot.OotAge <= LINK_AGE_CHILD) {
        gSaveContext.linkAge = static_cast<int32_t>(*boot.OotAge);
    }
    if (!boot.BootEntrance.has_value()) {
        return;
    }

    sBootEntranceApplied = true;
    sBootRespawnPending = false;
    sLoadSpawnDestination.reset();
    sLoadSpawnCrossGameSource.reset();
    sLoadSpawnHandoffReady = false;
    sGrottoExitSource = 0;
    gSaveContext.respawnFlag = 0;
    const auto target = ResolveOotEntrance(*boot.BootEntrance);
    if (!target.has_value()) {
        SPDLOG_ERROR("[OoTMM] Unsupported OoT boot entrance {}", *boot.BootEntrance);
        return;
    }
    gSaveContext.entranceIndex = static_cast<int16_t>(*target);
    gSaveContext.cutsceneIndex = 0;
    sBootRespawnPending = gSaveContext.respawnFlag != 0;
}

uint32_t CurrentAgeSpawnSource() {
    return gSaveContext.linkAge == LINK_AGE_ADULT ? 0x0F20 : 0x00BB;
}

bool SeedHasEntranceShuffle() {
    static constexpr const char* kBoolSettings[] = {
        "erMajorDungeons",   "erMinorDungeons",   "erSpiderHouses",  "erPirateFortress",
        "erBeneathWell",     "erIkanaCastle",     "erSecretShrine",  "erGanonCastle",
        "erGanonTower",      "erMoon",            "erIndoorsMajor",  "erIndoorsExtra",
        "erIndoorsTelescopes", "erOneWaysMajor",  "erOneWaysIkana",  "erOneWaysSongs",
        "erOneWaysStatues",  "erOneWaysOwls",
    };
    for (const char* setting : kBoolSettings) {
        if (sGameState.GetBoolSetting(setting, false)) {
            return true;
        }
    }

    static constexpr const char* kEnumSettings[] = {
        "erBoss", "erRegions", "erWarps", "erGrottos", "erWallmasters", "erOverworld",
    };
    for (const char* setting : kEnumSettings) {
        if (sGameState.GetStringSetting(setting, "none") != "none") {
            return true;
        }
    }
    return false;
}

void ApplySpawnForCurrentAge() {
    const uint32_t source = CurrentAgeSpawnSource();
    const auto* mapping = sGameState.FindEntrance(Ship::OotmmGame::Oot, source);
    if (mapping != nullptr && mapping->ToGame == Ship::OotmmGame::Mm) {
        sLoadSpawnDestination.reset();
        sLoadSpawnCrossGameSource = source;
        SPDLOG_INFO("[OoTMM] Current-age spawn 0x{:X} crosses to MM", source);
        return;
    }

    const uint32_t destination =
        mapping != nullptr && mapping->ToNativeId.has_value() ? *mapping->ToNativeId : source;
    if (const auto target = ResolveOotEntrance(destination); target.has_value()) {
        gSaveContext.entranceIndex = static_cast<int16_t>(*target);
        sBootRespawnPending = gSaveContext.respawnFlag != 0;
        sLoadSpawnDestination = destination;
        sLoadSpawnCrossGameSource.reset();
        SPDLOG_INFO("[OoTMM] Current-age spawn 0x{:X} maps to 0x{:X} (resolved 0x{:X})", source, destination,
                    *target);
    } else {
        SPDLOG_ERROR("[OoTMM] Unsupported OoT spawn target {}", destination);
    }
}

void ApplySeedSpawn() {
    sLoadSpawnHandoffReady = false;
    ApplySpawnForCurrentAge();
}

void RestoreLoadSpawn() {
    if (!sLoadSpawnDestination.has_value()) {
        return;
    }
    const uint32_t destination = *sLoadSpawnDestination;
    sLoadSpawnDestination.reset();
    if (const auto target = ResolveOotEntrance(destination); target.has_value()) {
        gSaveContext.entranceIndex = static_cast<int16_t>(*target);
        sBootRespawnPending = gSaveContext.respawnFlag != 0;
    }
}

void UpdateEntranceTransition() {
    if (gPlayState == nullptr) {
        return;
    }
    if (sLoadSpawnCrossGameSource.has_value()) {
        const auto* mapping =
            sGameState.FindEntrance(Ship::OotmmGame::Oot, *sLoadSpawnCrossGameSource);
        if (mapping == nullptr || mapping->ToGame != Ship::OotmmGame::Mm || !mapping->ToNativeId.has_value()) {
            SPDLOG_ERROR("[OoTMM] Cross-game load spawn mapping is no longer available");
            sLoadSpawnCrossGameSource.reset();
            sCrossGameWaitFrames = 0;
            return;
        }
        if (OotmmIpc_SendCrossGameTransition(*mapping, static_cast<uint32_t>(gSaveContext.linkAge))) {
            sLoadSpawnCrossGameSource.reset();
            sCrossGamePending = true;
            sCrossGameAccepted = false;
            sCrossGameWaitFrames = 0;
        } else if (++sCrossGameWaitFrames > 120) {
            SPDLOG_ERROR("[OoTMM] Cross-game load spawn requires the launcher IPC connection");
            sLoadSpawnCrossGameSource.reset();
            sCrossGameWaitFrames = 0;
        }
        return;
    }
    if (sCrossGamePending) {
        gPlayState->transitionTrigger = TRANS_TRIGGER_OFF;
        sCrossGameAccepted |= OotmmIpc_ConsumeTransitionAccepted();
        if (!sCrossGameAccepted && ++sCrossGameWaitFrames > 120) {
            SPDLOG_ERROR("[OoTMM] Launcher did not accept the cross-game transition");
            sCrossGamePending = false;
            sCrossGameWaitFrames = 0;
        }
        return;
    }
    if (gPlayState->transitionTrigger != TRANS_TRIGGER_START) {
        sPlayerExitPending = false;
        sPendingExtendedSource.reset();
        sLastResolvedEntrance.reset();
        return;
    }

    const bool playerExit = sPlayerExitPending;
    sPlayerExitPending = false;
    const uint32_t nextEntrance = static_cast<uint16_t>(gPlayState->nextEntranceIndex);
    if (sLastResolvedEntrance.has_value() &&
        (nextEntrance == *sLastResolvedEntrance || sPendingExtendedSource == sLastResolvedEntrance)) {
        return;
    }
    if (!playerExit && (gSaveContext.respawnFlag == 1 || gSaveContext.respawnFlag == 2 ||
                        gSaveContext.respawnFlag == 3 || gSaveContext.respawnFlag < 0) &&
        IsResolvedReloadTarget(nextEntrance)) {
        sLastResolvedEntrance = nextEntrance;
        sPendingExtendedSource.reset();
        return;
    }
    if (!sPendingExtendedSource.has_value() && nextEntrance < ENTR_MAX &&
        gEntranceTable[nextEntrance].scene == gPlayState->sceneNum) {
        sLastResolvedEntrance = nextEntrance;
        sGrottoReturnEntrance = 0;
        return;
    }

    sGrottoReturnEntrance = 0;
    uint32_t source = sPendingExtendedSource.value_or(nextEntrance);
    sPendingExtendedSource.reset();
    if (source == nextEntrance) {
        const uint32_t grottoEntry =
            TranslateGrottoEntry(static_cast<uint16_t>(nextEntrance), gPlayState->sceneNum,
                                 gSaveContext.respawn[RESPAWN_MODE_RETURN].data);
        if (grottoEntry != 0) {
            source = grottoEntry;
        }
    }
    const auto* mapping = sGameState.FindEntrance(Ship::OotmmGame::Oot, source);
    if (mapping == nullptr || !mapping->ToNativeId.has_value()) {
        return;
    }
    if (!mapping->IsCrossGame()) {
        if (const auto target = ResolveOotEntrance(*mapping->ToNativeId); target.has_value()) {
            gPlayState->nextEntranceIndex = static_cast<int16_t>(*target);
            sLastResolvedEntrance = *target;
            if (gSaveContext.respawnFlag == -2 &&
                gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex == static_cast<int16_t>(nextEntrance)) {
                gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = static_cast<int16_t>(*target);
            }
        } else {
            SPDLOG_ERROR("[OoTMM] Unsupported OoT entrance target {}", *mapping->ToNativeId);
        }
        return;
    }

    SPDLOG_INFO("[OoTMM] Cross-game entrance 0x{:X} -> {} (0x{:X})", source, mapping->To,
                *mapping->ToNativeId);
    Play_PerformSave(gPlayState);
    SaveManager::Instance->ThreadPoolWait();
    RecordActiveSaveFile(gSaveContext.fileNum);
    if (!OotmmIpc_SendCrossGameTransition(*mapping, static_cast<uint32_t>(gSaveContext.linkAge))) {
        SPDLOG_ERROR("[OoTMM] Cross-game transition requires the launcher IPC connection");
        gPlayState->transitionTrigger = TRANS_TRIGGER_OFF;
        return;
    }

    sCrossGamePending = true;
    sCrossGameAccepted = false;
    sCrossGameWaitFrames = 0;
    sLastResolvedEntrance = source;
    gPlayState->transitionTrigger = TRANS_TRIGGER_OFF;
}

void InitializeNewSave() {
    OotmmItemApply_ResetLedgerForNewSave();
    const bool adult = sGameState.GetStringSetting("startingAge", "child") == "adult";
    gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
    gSaveContext.entranceIndex = adult ? ENTR_TEMPLE_OF_TIME_WARP_PAD : ENTR_LINKS_HOUSE_CHILD_SPAWN;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.savedSceneNum = -1;
    ApplySaveFlags();
    ApplySeedSpawn();
    ApplyBootEntrance();
}

void BootIntoGame(GameState* gameState) {
    // The launcher's logical slot is MM's save identity and says nothing about OoT's file number.
    int32_t fileNum = static_cast<int32_t>(sGameState.GetBootConfig().LogicalSlot.value_or(0));
    if (const auto active = RecordedActiveSaveFile(); active.has_value() && Save_Exist(*active) != 0) {
        fileNum = *active;
    }
    if (Save_Exist(fileNum) == 0) {
        for (int32_t slot = 0; slot < 3; slot++) {
            if (Save_Exist(slot) != 0) {
                fileNum = slot;
                break;
            }
        }
    }
    gSaveContext.fileNum = fileNum;
    gSaveContext.gameMode = GAMEMODE_NORMAL;

    const bool hasSave = Save_Exist(fileNum) != 0;
    if (hasSave) {
        Sram_OpenSave();
    } else {
        Sram_InitNewSave();
        InitializeNewSave();
    }

    if (!sBootRespawnPending) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = ENTR_LOAD_OPENING;
        gSaveContext.respawnFlag = 0;
    }
    sBootRespawnPending = false;
    gSaveContext.seqId = static_cast<uint8_t>(NA_BGM_DISABLED);
    gSaveContext.natureAmbienceId = 0xFF;
    gSaveContext.showTitleCard = true;
    gSaveContext.dogParams = 0;
    gSaveContext.timerState = TIMER_STATE_OFF;
    gSaveContext.subTimerState = SUBTIMER_STATE_OFF;
    for (auto& eventInfo : gSaveContext.eventInf) {
        eventInfo = 0;
    }
    gSaveContext.unk_13EE = 0x32;
    gSaveContext.nayrusLoveTimer = 0;
    gSaveContext.healthAccumulator = 0;
    gSaveContext.magicState = MAGIC_STATE_IDLE;
    gSaveContext.prevMagicState = MAGIC_STATE_IDLE;
    gSaveContext.forcedSeqId = NA_BGM_GENERAL_SFX;
    gSaveContext.skyboxTime = 0;
    gSaveContext.nextTransitionType = TRANS_NEXT_TYPE_DEFAULT;
    gSaveContext.nextCutsceneIndex = 0xFFEF;
    gSaveContext.cutsceneTrigger = 0;
    gSaveContext.chamberCutsceneNum = 0;
    gSaveContext.nextDayTime = 0xFFFF;
    gSaveContext.retainWeatherMode = 0;
    for (auto& buttonStatus : gSaveContext.buttonStatus) {
        buttonStatus = BTN_ENABLED;
    }
    gSaveContext.forceRisingButtonAlphas = 0;
    gSaveContext.unk_13E8 = 0;
    gSaveContext.unk_13EA = 0;
    gSaveContext.unk_13EC = 0;
    gSaveContext.magicCapacity = 0;
    gSaveContext.magicFillTarget = gSaveContext.magic;
    gSaveContext.magic = 0;
    gSaveContext.magicLevel = 0;
    gSaveContext.naviTimer = 0;

    gameState->running = false;
    SET_NEXT_GAMESTATE(gameState, Play_Init, PlayState);

    if (!hasSave) {
        Save_SaveFile();
        SaveManager::Instance->ThreadPoolWait();
    }
    GameInteractor_ExecuteOnLoadGame(fileNum);
}

} // namespace

void OotmmSession_Init() {
    if (sGameState.LoadFromEnvironment()) {
        ApplyEnhancements();
        OotmmItemPresentation_Init();
        OotmmChecks_Init();
        OotmmItemProbe_Init();
        OotmmItemApply_Init();
        OotmmCustomItems_Init();
        OotmmScales_Init();
        OotmmPresence_Init();
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(
            [](int32_t fileNum) {
                RecordActiveSaveFile(fileNum);
                RestoreLoadSpawn();
                ApplySaveFlags();
                ApplyBootEntrance();
            });
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnDeleteFile>(DeleteForeignSave);
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(UpdateEntranceTransition);
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

extern "C" int32_t OotmmSession_TryBootDirectly(void* gameState) {
    if (!OotmmSession_IsActive() || !sGameState.GetBootConfig().BootEntrance.has_value() || sBootedIntoGame ||
        gameState == nullptr) {
        return 0;
    }
    sBootedIntoGame = true;
    BootIntoGame(static_cast<GameState*>(gameState));
    return 1;
}

extern "C" void OotmmSession_InitializeNewSave(void) {
    if (OotmmSession_IsActive()) {
        InitializeNewSave();
    }
}

extern "C" void OotmmSession_FixLoadSpawn(void) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    sLoadSpawnDestination.reset();
    sLoadSpawnCrossGameSource.reset();
    sLoadSpawnHandoffReady = false;
    if (gSaveContext.savedSceneNum == SCENE_LINKS_HOUSE && !SeedHasEntranceShuffle()) {
        gSaveContext.entranceIndex = ENTR_LINKS_HOUSE_CHILD_SPAWN;
        SPDLOG_INFO("[OoTMM] Honoring Link's House save without entrance shuffle");
        return;
    }
    ApplySpawnForCurrentAge();
    sLoadSpawnHandoffReady = sLoadSpawnCrossGameSource.has_value();
}

extern "C" int32_t OotmmSession_TryStartLoadSpawnHandoff(void) {
    if (!sLoadSpawnHandoffReady) {
        return 0;
    }
    if (sCrossGamePending) {
        return 1;
    }

    const auto* mapping =
        sLoadSpawnCrossGameSource.has_value()
            ? sGameState.FindEntrance(Ship::OotmmGame::Oot, *sLoadSpawnCrossGameSource)
            : nullptr;
    if (mapping == nullptr || mapping->ToGame != Ship::OotmmGame::Mm || !mapping->ToNativeId.has_value()) {
        SPDLOG_ERROR("[OoTMM] Cross-game load spawn mapping is no longer available");
        sLoadSpawnCrossGameSource.reset();
        sLoadSpawnHandoffReady = false;
        sCrossGameWaitFrames = 0;
        return 0;
    }

    if (OotmmIpc_SendCrossGameTransition(*mapping, static_cast<uint32_t>(gSaveContext.linkAge))) {
        sLoadSpawnCrossGameSource.reset();
        sCrossGamePending = true;
        sCrossGameAccepted = false;
        sCrossGameWaitFrames = 0;
        SPDLOG_INFO("[OoTMM] Waiting in File Select for MM spawn handoff");
    } else if (++sCrossGameWaitFrames == 120) {
        SPDLOG_ERROR("[OoTMM] Still waiting for the launcher IPC connection for MM spawn handoff");
    }
    return 1;
}

extern "C" void OotmmSession_NotePlayerExitTransition(void) {
    if (OotmmSession_IsActive()) {
        sPlayerExitPending = true;
    }
}

extern "C" void OotmmSession_PrepareGrottoReturn(void) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    const uint32_t source = CurrentGrottoExitSource();
    if (source != 0) {
        sPendingExtendedSource = source;
        sGrottoExitSource = 0;
    }
}

extern "C" int32_t OotmmSession_ReturnToSpawn(void) {
    if (!OotmmSession_IsActive() || gPlayState == nullptr) {
        return 0;
    }
    ApplySpawnForCurrentAge();
    if (sLoadSpawnCrossGameSource.has_value()) {
        sLoadSpawnHandoffReady = true;
        return OotmmSession_TryStartLoadSpawnHandoff();
    }
    sLoadSpawnDestination.reset();
    gPlayState->nextEntranceIndex = gSaveContext.entranceIndex;
    gSaveContext.respawnFlag = 0;
    gPlayState->transitionTrigger = TRANS_TRIGGER_START;
    gPlayState->transitionType = TRANS_TYPE_FADE_BLACK;
    sLastResolvedEntrance = static_cast<uint16_t>(gSaveContext.entranceIndex);
    return 1;
}

extern "C" void OotmmSession_ApplyDeathRespawn(void) {
    if (!OotmmSession_IsActive() || gPlayState == nullptr || sGrottoReturnEntrance == 0 ||
        sGrottoReturnEntrance != static_cast<uint16_t>(gSaveContext.entranceIndex)) {
        return;
    }
    gPlayState->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_RETURN].entranceIndex;
    gSaveContext.respawnFlag = kRespawnGrottoPopOut;
}

extern "C" int32_t OotmmSession_IsSceneMasterQuest(int32_t sceneNum) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const char* member = MasterQuestMember(sceneNum);
    return member != nullptr && sGameState.WorldFlagContains("mqDungeons", member) ? 1 : 0;
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
