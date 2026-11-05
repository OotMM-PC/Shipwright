#include <math.h>

#include "global.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "overlays/actors/ovl_Demo_Effect/z_demo_effect.h"

Gfx* ResourceMgr_LoadGfxByName(const char* path);
}

#include "OotmmElegyShell.h"

extern "C" int16_t gOotmmElegyShellId = -1;

namespace {

constexpr const char* kShellDL = "__OTR__objects/mm_gameplay_keep/gElegyShellHumanDL";
constexpr u8 kStateInitialized = 0;
constexpr u8 kStateFadingIn = 1;
constexpr u8 kStateSolid = 2;
constexpr u16 kBgCheckFlags = 1 | 4;

OotmmElegyShell* sShell = nullptr;

ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_METAL,
        AT_NONE,
        AC_ON | AC_HARD | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_PLAYER | OC1_TYPE_1 | OC1_TYPE_2,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00100000, 0x00, 0x00 },
        { 0xF7CFFFFF, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON | BUMP_HOOKABLE,
        OCELEM_ON,
    },
    { 20, 60, 0, { 0, 0, 0 } },
};

Gfx sRenderModeNoneDL[] = {
    gsSPEndDisplayList(),
    gsSPEndDisplayList(),
    gsSPEndDisplayList(),
    gsSPEndDisplayList(),
};

Gfx sRenderModeXluDL[] = {
    gsDPSetRenderMode(AA_EN | Z_CMP | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
                          GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                      G_RM_AA_ZB_XLU_SURF2),
    gsSPEndDisplayList(),
    gsDPSetRenderMode(AA_EN | Z_CMP | IM_RD | CLR_ON_CVG | CVG_DST_WRAP | ZMODE_XLU | FORCE_BL |
                          GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA),
                      G_RM_AA_ZB_XLU_SURF2),
    gsSPEndDisplayList(),
};

} // namespace

extern "C" void OotmmElegyShell_Init(Actor* thisx, PlayState* play) {
    OotmmElegyShell* self = reinterpret_cast<OotmmElegyShell*>(thisx);
    self->actor.colChkInfo.mass = MASS_IMMOVABLE;
    Collider_InitCylinder(play, &self->collider);
    Collider_SetCylinder(play, &self->collider, &self->actor, &sCylinderInit);
    self->actor.flags |= ACTOR_FLAG_CAN_PRESS_SWITCHES;
    self->framesUntilNextState = 20;
    sShell = self;
}

extern "C" void OotmmElegyShell_Destroy(Actor* thisx, PlayState* play) {
    OotmmElegyShell* self = reinterpret_cast<OotmmElegyShell*>(thisx);
    Collider_DestroyCylinder(play, &self->collider);
    if (sShell == self) {
        sShell = nullptr;
    }
}

extern "C" void OotmmElegyShell_Update(Actor* thisx, PlayState* play) {
    OotmmElegyShell* self = reinterpret_cast<OotmmElegyShell*>(thisx);
    self->actor.gravity = -1.0f;
    Actor_MoveXZGravity(&self->actor);
    Actor_UpdateBgCheckInfo(play, &self->actor, 30.0f, 20.0f, 70.0f, kBgCheckFlags);

    if (self->framesUntilNextState != 0) {
        self->framesUntilNextState--;
    }
    if (self->framesUntilNextState != 0) {
        return;
    }

    s16 targetAlpha;
    if (self->state == kStateInitialized) {
        if (self->alpha == 0) {
            Math_Vec3f_Copy(&self->actor.world.pos, &self->actor.home.pos);
            self->actor.shape.rot.y = self->actor.home.rot.y;
            self->state = kStateFadingIn;
        }
        targetAlpha = 0;
    } else if (self->state == kStateFadingIn) {
        if (self->actor.xzDistToPlayer > 32.0f || fabsf(self->actor.yDistToPlayer) > 70.0f) {
            self->state = kStateSolid;
        }
        targetAlpha = 60;
    } else {
        Collider_UpdateCylinder(&self->actor, &self->collider);
        CollisionCheck_SetOC(play, &play->colChkCtx, &self->collider.base);
        targetAlpha = 255;
    }
    Math_StepToS(&self->alpha, targetAlpha, 8);
}

extern "C" void OotmmElegyShell_Draw(Actor* thisx, PlayState* play) {
    OotmmElegyShell* self = reinterpret_cast<OotmmElegyShell*>(thisx);
    Gfx* dl = ResourceMgr_LoadGfxByName(kShellDL);
    if (dl == nullptr) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);
    if (self->alpha == 255) {
        gSPSegment(POLY_OPA_DISP++, 0x0C, (uintptr_t)sRenderModeNoneDL);
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
        Gfx_DrawDListOpa(play, dl);
    } else {
        gSPSegment(POLY_XLU_DISP++, 0x0C, (uintptr_t)sRenderModeXluDL);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, self->alpha);
        Gfx_DrawDListXlu(play, dl);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void OotmmElegyShell_Place(PlayState* play) {
    Player* player = GET_PLAYER(play);
    if (sShell != nullptr) {
        Math_Vec3f_Copy(&sShell->actor.home.pos, &player->actor.world.pos);
        sShell->actor.home.rot.y = player->actor.shape.rot.y;
        sShell->state = kStateInitialized;
        sShell->framesUntilNextState = 20;
    } else {
        Actor_Spawn(&play->actorCtx, play, gOotmmElegyShellId, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0, 0);
    }

    DemoEffect* effect = reinterpret_cast<DemoEffect*>(
        Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_EFFECT, player->actor.world.pos.x, player->actor.world.pos.y,
                    player->actor.world.pos.z, 0, player->actor.shape.rot.y, 0, DEMO_EFFECT_TIMEWARP_TIMEBLOCK_SMALL));
    if (effect != nullptr) {
        effect->envXluColor[0] = 0;
        effect->envXluColor[1] = 100;
        effect->envXluColor[2] = 0;
    }
}
