extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/gameplay_keep/gameplay_keep.h"

void FrameInterpolation_RecordOpenChild(const void* a, int b);
void FrameInterpolation_RecordCloseChild(void);
}

#include "OotmmAqua.h"

extern "C" int16_t gOotmmAquaId = -1;

namespace {

constexpr int kAquaHot = 1;
constexpr u16 kBgGround = 1;
constexpr u16 kBgWater = 0x20;

ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_OTHER,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x00, 0x00 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NONE,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { 6, 10, 0, { 0, 0, 0 } },
};

InitChainEntry sInitChain[] = {
    ICHAIN_VEC3S(shape.rot, 0, ICHAIN_CONTINUE),          ICHAIN_VEC3S(world.rot, 0, ICHAIN_CONTINUE),
    ICHAIN_F32_DIV1000(gravity, -900, ICHAIN_CONTINUE),   ICHAIN_F32_DIV1000(minVelocityY, -4000, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneForward, 4000, ICHAIN_CONTINUE), ICHAIN_F32(uncullZoneScale, 300, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 300, ICHAIN_STOP),
};

Vec3f sFallingScale = { 1.0f / 1000.0f, 7.0f / 10000.0f, 1.0f / 1000.0f };
Vec3f sPuddleScale = { 8.6f / 1000.0f, 8.0f / 10000.0f, 8.6f / 1000.0f };
Vec3f sUnderwaterScale = { 1.0f / 100.0f, 2.6f / 1000.0f, 1.0f / 100.0f };

bool IsHot(OotmmAqua* self) {
    return (self->actor.params & 1) == kAquaHot;
}

void Splash(PlayState* play, OotmmAqua* self, f32 surfaceY) {
    Vec3f pos;
    s32 angleOffset = 0;
    pos.y = surfaceY;
    for (int i = 0; i < 4; i++) {
        pos.x = self->actor.world.pos.x + Math_SinS(static_cast<s32>(Rand_ZeroOne() * 7200.0f) + angleOffset) * 8.0f;
        pos.z = self->actor.world.pos.z + Math_CosS(static_cast<s32>(Rand_ZeroOne() * 7200.0f) + angleOffset) * 8.0f;
        EffectSsGSplash_Spawn(play, &pos, nullptr, nullptr, 0, 120);
        angleOffset += 0x4000;
    }
    pos.x = self->actor.world.pos.x;
    pos.z = self->actor.world.pos.z;
    EffectSsGSplash_Spawn(play, &pos, nullptr, nullptr, 0, 300);
}

void Steam(OotmmAqua* self, PlayState* play) {
    Vec3f pos;
    Vec3f velocity;
    Vec3f accel = { 0.0f, 0.0f, 0.0f };
    velocity.x = Rand_ZeroOne() - 0.5f;
    velocity.y = 2.0f;
    velocity.z = Rand_ZeroOne() - 0.5f;
    pos.x = self->actor.world.pos.x + (velocity.x * 40.0f);
    pos.y = self->actor.world.pos.y;
    pos.z = self->actor.world.pos.z + (velocity.z * 40.0f);
    EffectSsIceSmoke_Spawn(play, &pos, &velocity, &accel, static_cast<s32>(Rand_ZeroOne() * 24.0f) + 70);
}

void AlignToFloor(OotmmAqua* self) {
    MtxF mtx;
    func_80038A28(self->actor.floorPoly, self->actor.world.pos.x, self->actor.floorHeight, self->actor.world.pos.z,
                  &mtx);
    Matrix_MtxFToYXZRotS(&mtx, &self->actor.shape.rot, false);
}

bool IsUnderwater(OotmmAqua* self, PlayState* play) {
    WaterBox* waterBox;
    f32 surface;
    return WaterBox_GetSurfaceImpl(play, &play->colCtx, self->actor.world.pos.x, self->actor.world.pos.z, &surface,
                                   &waterBox) &&
           self->actor.world.pos.y < surface;
}

void Fall(OotmmAqua* self, PlayState* play);
void Puddle(OotmmAqua* self, PlayState* play);
void Sink(OotmmAqua* self, PlayState* play);

void Fall(OotmmAqua* self, PlayState* play) {
    if (self->actor.bgCheckFlags & (kBgGround | kBgWater)) {
        if (self->actor.bgCheckFlags & kBgGround) {
            Splash(play, self, self->actor.floorHeight);
            AlignToFloor(self);
            Audio_PlayActorSound2(&self->actor, NA_SE_EV_BOMB_DROP_WATER);
            self->actionFunc = Puddle;
        } else {
            Splash(play, self, self->actor.world.pos.y + self->actor.yDistToWater);
            SoundSource_PlaySfxAtFixedWorldPos(play, &self->actor.world.pos, 40, NA_SE_EV_BOMB_DROP_WATER);
            Actor_Kill(&self->actor);
        }
    } else if (self->counter <= 0) {
        Actor_Kill(&self->actor);
    }
}

void Puddle(OotmmAqua* self, PlayState* play) {
    if (IsHot(self) && self->alpha > 90) {
        Steam(self, play);
    }
    self->alpha = self->alpha > 5 ? self->alpha - 5 : 0;
    if (self->actor.shape.shadowAlpha > 2) {
        self->actor.shape.shadowAlpha -= 2;
    } else {
        Actor_Kill(&self->actor);
    }
}

void Sink(OotmmAqua* self, PlayState* play) {
    self->alpha = static_cast<u8>(static_cast<s32>(self->counter * 3.25f) + 10);
    self->actor.shape.shadowAlpha = self->alpha;
    self->wobble += 1000;
    if (IsHot(self)) {
        const f32 size = self->actor.scale.x * 10000.0f;
        EffectSsBubble_Spawn(play, &self->actor.world.pos, size * -0.5f, size, size, (Rand_ZeroOne() * 0.1f) + 0.03f);
    }
    if (self->counter <= 0) {
        Actor_Kill(&self->actor);
    }
}

void StepScale(Vec3f* scale, Vec3f target, f32 speed) {
    Vec3f diff;
    Math_Vec3f_Diff(&target, scale, &diff);
    const f32 distance = sqrtf(SQ(diff.x) + SQ(diff.y) + SQ(diff.z));
    if (speed < distance) {
        const f32 step = speed / distance;
        scale->x += step * diff.x;
        scale->y += step * diff.y;
        scale->z += step * diff.z;
    } else {
        *scale = target;
    }
}

} // namespace

extern "C" void OotmmAqua_Init(Actor* thisx, PlayState* play) {
    OotmmAqua* self = reinterpret_cast<OotmmAqua*>(thisx);

    Actor_ProcessInitChain(&self->actor, sInitChain);
    self->actor.scale.x = 0.0009f;
    self->actor.scale.y = 0.0005f;
    self->actor.scale.z = 0.0009f;
    Collider_InitCylinder(play, &self->collider);
    Collider_SetCylinder(play, &self->collider, &self->actor, &sCylinderInit);
    ActorShape_Init(&self->actor.shape, 0.0f, ActorShadow_DrawCircle, 60.0f);
    self->actor.shape.shadowAlpha = 140;
    self->alpha = 255;
    self->wobble = 0;
    if (IsUnderwater(self, play)) {
        for (int i = 0; i < 8; i++) {
            EffectSsBubble_Spawn(play, &self->actor.world.pos, -4.0f, 4.0f, 4.0f, (Rand_ZeroOne() * 0.09f) + 0.03f);
        }
        self->actor.gravity = -0.1f;
        self->counter = 40;
        self->alpha = 140;
        self->actionFunc = Sink;
    } else {
        self->counter = 200;
        self->actionFunc = Fall;
    }
}

extern "C" void OotmmAqua_Destroy(Actor* thisx, PlayState* play) {
    OotmmAqua* self = reinterpret_cast<OotmmAqua*>(thisx);
    Collider_DestroyCylinder(play, &self->collider);
}

extern "C" void OotmmAqua_Update(Actor* thisx, PlayState* play) {
    OotmmAqua* self = reinterpret_cast<OotmmAqua*>(thisx);

    if (self->counter > 0) {
        self->counter--;
    }
    self->actionFunc(self, play);
    if (self->actor.update == nullptr) {
        return;
    }
    if (self->actionFunc == Fall) {
        StepScale(&self->actor.scale, sFallingScale, 0.00006f);
    } else if (self->actionFunc == Puddle) {
        StepScale(&self->actor.scale, sPuddleScale, 0.00095f);
    } else {
        StepScale(&self->actor.scale, sUnderwaterScale, 0.0004f);
    }
    self->actor.velocity.y *= 0.9f;
    Actor_MoveXZGravity(&self->actor);
    Actor_UpdateBgCheckInfo(play, &self->actor, 12.0f, 4.0f, 0.0f, 1 | 4);
    if (self->actionFunc != Sink) {
        Collider_UpdateCylinder(&self->actor, &self->collider);
        self->collider.dim.radius = static_cast<s16>(self->actor.scale.x * 3000.0f);
        CollisionCheck_SetAT(play, &play->colChkCtx, &self->collider.base);
    }
}

extern "C" void OotmmAqua_Draw(Actor* thisx, PlayState* play) {
    OotmmAqua* self = reinterpret_cast<OotmmAqua*>(thisx);
    const s16 yaw = Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)) + 0x8000;
    const bool sinking = self->actionFunc == Sink;
    s32 scroll = ((play->gameplayFrames & 0x7FFFFFFF) * -0xA) & 0x1FF;
    if (sinking) {
        scroll >>= 1;
    }

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               reinterpret_cast<uintptr_t>(
                   Gfx_TwoTexScroll(play->state.gfxCtx, 0, 0, 0, 0x20, 0x40, 1, 0, scroll, 0x20, 0x80)));
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 170, 255, 255, self->alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, 0, 150, 255, 0);
    if (sinking) {
        const s16 rotation = static_cast<s16>(Math_SinS(self->wobble) * 8000.0f);
        Matrix_RotateZ(BINANG_TO_RAD(rotation), MTXMODE_APPLY);
        Matrix_Scale(1.3f, 1.0f, 1.0f, MTXMODE_APPLY);
        Matrix_RotateZ(BINANG_TO_RAD(-rotation), MTXMODE_APPLY);
        Matrix_Scale(10.0f / 13.0f, 1.0f, 1.0f, MTXMODE_APPLY);
    }
    Matrix_RotateY(BINANG_TO_RAD(yaw), MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, reinterpret_cast<Gfx*>(const_cast<char*>(gEffFire1DL)));
    CLOSE_DISPS(play->state.gfxCtx);
}
