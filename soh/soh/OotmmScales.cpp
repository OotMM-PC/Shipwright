#include "OotmmScales.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

#include <libultraship/bridge/OotmmScales.h>

#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include <cstdint>

extern "C" {
#include "variables.h"
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "sfx.h"

extern PlayState* gPlayState;
extern f32 sFontWidths[144];
extern const char* fontTbl[140];
}

// z_play.c has no public prototype for this.
extern "C" void Play_SetRespawnData(PlayState* play, s32 respawnMode, s16 entranceIndex, s32 roomIndex,
                                    s32 playerParams, Vec3f* pos, s16 yaw);

namespace {

// SoH spells bgCheckFlags out as raw values: bit 0 = standing on ground, bit 5 = touching a water box.
constexpr uint32_t kBgOnGround = 0x1;
constexpr uint32_t kBgInWater = 0x20;
// Paces only the no-ground fallback; a respawn that is itself wet would re-void every frame.
constexpr uint8_t kNoGroundVoidFrames = 20;
constexpr int32_t kVoidPlayerParams = 0xDFF;

struct SafeGround {
    bool Valid = false;
    Vec3f Pos{};
    int16_t Yaw = 0;
    int16_t EntranceIndex = 0;
    int32_t Room = 0;
};

SafeGround sSafe;
uint8_t sNoGroundVoidTimer = 0;
uint8_t sConsecutiveVoids = 0;

bool SwimNeedsScale() {
    return OotmmSession_IsActive() &&
           Ship::OotmmSwimNeedsScale(OotmmSession_GetState(), Ship::OotmmGame::Oot);
}

bool OnStandableGround(Player* player) {
    return (player->actor.bgCheckFlags & kBgOnGround) && !(player->actor.bgCheckFlags & kBgInWater) &&
           !(player->stateFlags1 & PLAYER_STATE1_IN_WATER) && player->actor.floorBgId == BGCHECK_SCENE;
}

// A frame into a scene change gSaveContext.entranceIndex already names the next scene, and voiding
// to that entrance with this scene's coordinates would drop Link somewhere else entirely.
bool EntranceLoadsCurrentScene(PlayState* play) {
    const uint16_t entrance = static_cast<uint16_t>(gSaveContext.entranceIndex);
    return entrance < static_cast<uint16_t>(ENTR_MAX) && gEntranceTable[entrance].scene == play->sceneNum;
}

void CaptureGround(PlayState* play, Player* player) {
    sSafe.Valid = true;
    sSafe.Pos = player->actor.world.pos;
    // Flipped so the void leaves Link facing away from the doorway he was standing in.
    sSafe.Yaw = static_cast<int16_t>(player->actor.shape.rot.y + 0x8000);
    sSafe.EntranceIndex = static_cast<int16_t>(gSaveContext.entranceIndex);
    sSafe.Room = play->roomCtx.curRoom.num;
    sNoGroundVoidTimer = 0;
    sConsecutiveVoids = 0;
}

void VoidToDryGround(PlayState* play) {
    Sfx_PlaySfxCentered(NA_SE_EV_WATER_CONVECTION);

    // Draining a heart per repeat hands an underwater respawn to the death sequence rather than
    // an endless fade loop.
    if (++sConsecutiveVoids >= 2) {
        gSaveContext.health -= 16;
        if (gSaveContext.health <= 0) {
            gSaveContext.health = 0;
            return;
        }
    }

    if (sSafe.Valid) {
        Play_SetRespawnData(play, RESPAWN_MODE_DOWN, sSafe.EntranceIndex, sSafe.Room, kVoidPlayerParams, &sSafe.Pos,
                            sSafe.Yaw);
    }
    Play_TriggerVoidOut(play);
}

void OnPlayerUpdate() {
    if (gPlayState == nullptr || !SwimNeedsScale()) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return;
    }

    if (gPlayState->transitionTrigger == TRANS_TRIGGER_OFF && EntranceLoadsCurrentScene(gPlayState) &&
        OnStandableGround(player)) {
        CaptureGround(gPlayState, player);
        return;
    }

    if (OotmmScales_Tier() != OOTMM_SCALE_NONE) {
        return;
    }
    // Iron boots walk the floor rather than swim.
    if (CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS) == EQUIP_VALUE_BOOTS_IRON) {
        return;
    }
    if (gPlayState->transitionTrigger != TRANS_TRIGGER_OFF) {
        return;
    }
    if (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_IN_ITEM_CS |
                               PLAYER_STATE1_TALKING | PLAYER_STATE1_GETTING_ITEM)) {
        return;
    }
    // Voiding under an open prompt would discard an armed get-item or save dialog.
    if (gPlayState->pauseCtx.state != 0 || Message_GetState(&gPlayState->msgCtx) != TEXT_STATE_NONE) {
        return;
    }
    if (!(player->stateFlags1 & PLAYER_STATE1_IN_WATER)) {
        sNoGroundVoidTimer = 0;
        return;
    }

    if (!sSafe.Valid) {
        if (sNoGroundVoidTimer < kNoGroundVoidFrames) {
            sNoGroundVoidTimer++;
            return;
        }
        sNoGroundVoidTimer = 0;
    }
    VoidToDryGround(gPlayState);
}

// Matches the bronze recolor the item model uses, so the icon and the 3D scale read as one item.
constexpr uint8_t kBronzeTint[3] = { 255, 200, 100 };
constexpr char kBronzeName[] = "Bronze Scale";

} // namespace

extern "C" int OotmmScales_DrawSlotIcon(struct PlayState* play) {
    if (OotmmScales_Tier() != OOTMM_SCALE_BRONZE) {
        return 0;
    }

    OPEN_DISPS(play->state.gfxCtx);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, kBronzeTint[0], kBronzeTint[1], kBronzeTint[2], play->pauseCtx.alpha);
    gDPLoadTextureBlock(POLY_OPA_DISP++, gItemIcons[ITEM_SCALE_SILVER], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);
    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, play->pauseCtx.alpha);
    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}

extern "C" int OotmmScales_DrawSlotName(struct PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    if (pauseCtx->namedItem != ITEM_OOTMM_SCALE_BRONZE) {
        return 0;
    }

    const size_t length = sizeof(kBronzeName) - 1;
    f32 width = 0.0f;
    for (size_t i = 0; i < length; i++) {
        width += sFontWidths[kBronzeName[i] - ' '];
    }

    // Frame-scoped so every glyph keeps its own vertices until the display list runs.
    Vtx* verts = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, sizeof(Vtx) * 4 * length));
    if (verts == nullptr) {
        return 0;
    }

    OPEN_DISPS(play->state.gfxCtx);
    gDPPipeSync(POLY_OPA_DISP++);

    f32 x = -width / 2.0f;
    for (size_t i = 0; i < length; i++) {
        const char c = kBronzeName[i];
        if (c != ' ') {
            Vtx* quad = &verts[i * 4];
            for (size_t v = 0; v < 4; v++) {
                quad[v] = pauseCtx->infoPanelVtx[16 + v];
            }
            quad[0].v.ob[0] = quad[2].v.ob[0] = static_cast<s16>(x);
            quad[1].v.ob[0] = quad[3].v.ob[0] = static_cast<s16>(x) + FONT_CHAR_TEX_WIDTH;
            quad[0].v.tc[0] = quad[2].v.tc[0] = 0;
            quad[1].v.tc[0] = quad[3].v.tc[0] = FONT_CHAR_TEX_WIDTH << 5;

            gSPVertex(POLY_OPA_DISP++, reinterpret_cast<uintptr_t>(quad), 4, 0);
            gDPLoadTextureBlock_4b(POLY_OPA_DISP++, fontTbl[c - ' '], G_IM_FMT_I, FONT_CHAR_TEX_WIDTH,
                                   FONT_CHAR_TEX_HEIGHT, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                                   G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
        }
        x += sFontWidths[c - ' '];
    }

    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}

extern "C" int OotmmScales_SlotFilled(void) {
    return OotmmScales_Tier() != OOTMM_SCALE_NONE;
}

extern "C" void OotmmScales_Init(void) {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(OnPlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) {
        sSafe = SafeGround{};
        sNoGroundVoidTimer = 0;
        sConsecutiveVoids = 0;
    });
}

extern "C" int OotmmScales_Tier(void) {
    if (!OotmmSession_IsActive()) {
        return static_cast<int>(Ship::OotmmScaleTier::None);
    }
    const Ship::OotmmScaleTier tier =
        Ship::OotmmScaleTierOf(OotmmSession_GetState(), OotmmIpc_GetInventory(), Ship::OotmmGame::Oot,
                               CUR_UPG_VALUE(UPG_SCALE));
    return static_cast<int>(tier);
}
