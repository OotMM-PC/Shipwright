#include "OotmmRegionState.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

#include <string>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/gameplay_keep/gameplay_keep.h"

extern PlayState* gPlayState;
}

s16 gOotmmCustomWarpId = -1;

namespace {

constexpr int kDungeonDekuTree = 0;
constexpr int kDungeonWaterTemple = 5;
constexpr int kEventDekuTreeDead = 0x17;
constexpr int kClearEffect = 1 << 1;
constexpr int kClearWisp = 1 << 2;
constexpr s16 kWarpLakeHyliaWater = 0;

bool sWarpSpawned = false;

const Ship::OotmmGameState& State() {
    return OotmmSession_GetState();
}

bool LakeWispEnabled() {
    const std::string mode = State().GetStringSetting("regionState", "dungeonBeaten");
    if (mode == "free") {
        return true;
    }
    if (mode == "reward" && CHECK_QUEST_ITEM(QUEST_MEDALLION_WATER)) {
        return true;
    }
    return gSaveContext.ship.ootmmWispLake != 0;
}

void SpawnWarps(PlayState* play) {
    if (gOotmmCustomWarpId < 0 || play->sceneNum != SCENE_LAKE_HYLIA || !LINK_IS_ADULT || !LakeWispEnabled()) {
        return;
    }
    sWarpSpawned = Actor_Spawn(&play->actorCtx, play, gOotmmCustomWarpId, -850.0f, -1223.0f, 6950.0f, 0, 0, 0,
                               kWarpLakeHyliaWater) != nullptr;
}

void TriggerWarp(PlayState* play, s16 params) {
    if (params != kWarpLakeHyliaWater) {
        return;
    }
    if (Flags_GetEventChkInf(EVENTCHKINF_RAISED_LAKE_HYLIA_WATER)) {
        Flags_UnsetEventChkInf(EVENTCHKINF_RAISED_LAKE_HYLIA_WATER);
    } else {
        Flags_SetEventChkInf(EVENTCHKINF_RAISED_LAKE_HYLIA_WATER);
    }
    Play_SetupRespawnPoint(play, RESPAWN_MODE_RETURN, 0xDFF);
    gSaveContext.respawnFlag = 2;
    play->nextEntranceIndex = gSaveContext.entranceIndex;
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
}

} // namespace

extern "C" void OotmmCustomWarp_Init(Actor* thisx, PlayState* play) {
    thisx->room = -1;
}

extern "C" void OotmmCustomWarp_Update(Actor* thisx, PlayState* play) {
    if (Actor_ProcessTalkRequest(thisx, play)) {
        TriggerWarp(play, thisx->params);
    } else {
        func_8002F2CC(thisx, play, 30.0f);
    }
}

extern "C" void OotmmCustomWarp_Draw(Actor* thisx, PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Translate(thisx->world.pos.x, thisx->world.pos.y, thisx->world.pos.z, MTXMODE_NEW);
    Matrix_Scale(0.003f, 0.003f, 0.003f, MTXMODE_APPLY);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               reinterpret_cast<uintptr_t>(Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, 0, 0x20, 0x40, 1, 0,
                                                            (-play->gameplayFrames & 0x7F) << 2, 0x20, 0x80)));
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 0, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 0, 0, 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gEffFire1DL);

    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void OotmmRegionState_SetDungeonFlags(int dungeonId, int flags) {
    switch (dungeonId) {
        case kDungeonDekuTree:
            Flags_SetEventChkInf(EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD);
            Flags_SetEventChkInf(EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP);
            Flags_SetEventChkInf(kEventDekuTreeDead);
            Flags_SetEventChkInf(EVENTCHKINF_SPOKE_TO_MIDO_AFTER_DEKU_TREES_DEATH);
            break;
        case kDungeonWaterTemple:
            if (flags & kClearEffect) {
                Flags_SetEventChkInf(EVENTCHKINF_RAISED_LAKE_HYLIA_WATER);
                gSaveContext.ship.ootmmWispLake = 1;
            }
            if (flags & kClearWisp) {
                gSaveContext.ship.ootmmWispLake = 1;
            }
            break;
        default:
            break;
    }
}

extern "C" void OotmmRegionState_ApplyBossWarp(int bossId) {
    const auto dungeon = State().GetBossDungeon(static_cast<uint32_t>(bossId));
    if (!dungeon.has_value()) {
        return;
    }
    int flags = 0;
    if (State().GetStringSetting("regionState", "dungeonBeaten") == "dungeonBeaten") {
        flags = kClearEffect | (State().GetBoolSetting("erMajorDungeons", false) ? kClearWisp : 0);
    }
    OotmmRegionState_SetDungeonFlags(static_cast<int>(*dungeon), flags);
}

void OotmmRegionState_InitializeNewSave() {
    const int flags = State().GetStringSetting("regionState", "dungeonBeaten") == "dungeonBeaten" ? kClearWisp : 0;
    if (State().IsDungeonPreCompleted("DT")) {
        OotmmRegionState_SetDungeonFlags(kDungeonDekuTree, flags);
    }
    if (State().IsDungeonPreCompleted("Water")) {
        OotmmRegionState_SetDungeonFlags(kDungeonWaterTemple, flags);
    }
}

void OotmmRegionState_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { sWarpSpawned = false; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([]() {
        if (!sWarpSpawned && OotmmSession_IsActive() && gPlayState != nullptr) {
            SpawnWarps(gPlayState);
        }
    });
}
