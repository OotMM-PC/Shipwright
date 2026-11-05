#include <math.h>

#include "global.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "OotmmCustomItemsPlayer.h"
}

#include "soh/frame_interpolation.h"
#include "OotmmBomMm.h"
#include "OotmmClearTag.h"

extern "C" int16_t gOotmmBomMmId = -1;

namespace {

constexpr s16 kBombTypeBody = 0;
constexpr s16 kBombTypeExplosion = 1;
constexpr u16 kBgCheckGround = 1;
constexpr u16 kBgCheckGroundTouch = 2;
constexpr u16 kBgCheckWall = 8;
constexpr u16 kBgCheckCeiling = 0x10;
constexpr u16 kBgCheckWaterTouch = 0x40;
constexpr u32 kFloorTypeSand = 4;
constexpr u32 kFloorType14 = 14;
constexpr u32 kFloorType15 = 15;
constexpr f32 kPi = static_cast<f32>(M_PI);

constexpr const char* kKegFuseMaterialDL = "__OTR__overlays/mm_ovl_En_Bom/gPowderKegFuseMaterialDL";
constexpr const char* kKegFuseModel1DL = "__OTR__overlays/mm_ovl_En_Bom/gPowderKegFuseModel1DL";
constexpr const char* kKegFuseModel2DL = "__OTR__overlays/mm_ovl_En_Bom/gPowderKegFuseModel2DL";
constexpr const char* kKegBarrelDL = "__OTR__overlays/mm_ovl_En_Bom/gPowderKegBarrelDL";
constexpr const char* kKegSkullDL = "__OTR__overlays/mm_ovl_En_Bom/gPowderKegGoronSkullDL";

const f32 sBombScales[] = { 0.01f, 0.03f };
const s16 sQuakeY[] = { 3, 5 };
const s16 sQuakeDurations[] = { 10, 15 };
const s16 sExplosionRadius[] = { 100, 200 };
const Color_RGBA8 sSandDustColor = { 185, 140, 70, 255 };

struct FlashTimings {
    u16 first;
    u16 second;
    u16 start;
};
const FlashTimings sFlashTimings[] = {
    { 40, 20, 100 },
    { 300, 60, 600 },
};

struct BombPhysics {
    f32 deceleration;
    f32 bounceThreshold;
    f32 bounceAmount;
};
const BombPhysics sBombPhysics[] = {
    { 2.0f, -6.0f, -0.3f },
    { 1.5f, -5.0f, -0.6f },
};

struct PowderKegFuseSegment {
    Vec3f pos;
    Vec3f velocity;
    s16 rotY;
    s16 rotX;
};
PowderKegFuseSegment sFuseSegments[16];

Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };
Vec3f sBombCapOffset = { 0.0f, 800.0f, 0.0f };
Vec3f sFuseRootOffset = { -750.0f, 0.0f, 0.0f };
Vec3f sFuseTipOffset = { -800.0f, 0.0f, 0.0f };

ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_HIT0,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER | AC_TYPE_OTHER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_2,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000000, 0x00, 0x00 },
        { 0x0003F828, 0x00, 0x00 },
        TOUCH_NONE | TOUCH_SFX_NORMAL,
        BUMP_ON,
        OCELEM_ON,
    },
    { 6, 11, 14, { 0, 0, 0 } },
};

ColliderJntSphElementInit sBombSphereElements[1] = {
    {
        {
            ELEMTYPE_UNK0,
            { 0x00000008, 0x00, 0x08 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NONE,
            BUMP_NONE,
            OCELEM_NONE,
        },
        { 0, { { 0, 0, 0 }, 0 }, 100 },
    },
};

ColliderJntSphElementInit sKegSphereElements[1] = {
    {
        {
            ELEMTYPE_UNK0,
            { 0x40000008, 0x00, 0x08 },
            { 0x00000000, 0x00, 0x00 },
            TOUCH_ON | TOUCH_SFX_NONE,
            BUMP_NONE,
            OCELEM_NONE,
        },
        { 0, { { 0, 0, 0 }, 0 }, 100 },
    },
};

ColliderJntSphInit sBombSphereInit = {
    {
        COLTYPE_HIT0,
        AT_ON | AT_TYPE_ALL,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_JNTSPH,
    },
    1,
    sBombSphereElements,
};

ColliderJntSphInit sKegSphereInit = {
    {
        COLTYPE_HIT0,
        AT_ON | AT_TYPE_ALL,
        AC_NONE,
        OC1_NONE,
        OC2_NONE,
        COLSHAPE_JNTSPH,
    },
    1,
    sKegSphereElements,
};

InitChainEntry sInitChain[] = {
    ICHAIN_VEC3F(scale, 0, ICHAIN_CONTINUE),
    ICHAIN_F32(targetArrowOffset, 2000, ICHAIN_CONTINUE),
    ICHAIN_F32_DIV1000(gravity, -4000, ICHAIN_STOP),
};

void Move(OotmmBomMm* self, PlayState* play);
void WaitForRelease(OotmmBomMm* self, PlayState* play);
void Explode(OotmmBomMm* self, PlayState* play);

s16 BinangSub(s16 a, s16 b) {
    return static_cast<s16>(a - b);
}

void MultVecZ(f32 z, Vec3f* dest) {
    Vec3f src = { 0.0f, 0.0f, z };
    Matrix_MultVec3f(&src, dest);
}

void RotateYS(s16 angle, u8 mode) {
    Matrix_RotateY(angle * (kPi / 32768.0f), mode);
}

void RotateXS(s16 angle, u8 mode) {
    Matrix_RotateX(angle * (kPi / 32768.0f), mode);
}

void Vec3sToVec3f(Vec3f* dest, const Vec3s* src) {
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
}

void Vec3fToVec3s(Vec3s* dest, const Vec3f* src) {
    dest->x = static_cast<s16>(src->x);
    dest->y = static_cast<s16>(src->y);
    dest->z = static_cast<s16>(src->z);
}

void ReleaseFromPlayer(PlayState* play, Actor* actor) {
    Player* player = GET_PLAYER(play);
    if ((player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) && (player->heldActor == actor)) {
        player->interactRangeActor = nullptr;
        player->heldActor = nullptr;
        player->actor.child = nullptr;
        player->stateFlags1 &= ~PLAYER_STATE1_CARRYING_ACTOR;
    }
}

void PlaySfxWithChannelIO(Vec3f* pos, u16 sfxId, u8 ioData) {
    func_800F4C58(pos, sfxId, ioData);
    Audio_PlaySoundGeneral(sfxId, pos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultReverb);
}

void InitKegFuseSegments(Vec3f* bombPos) {
    for (PowderKegFuseSegment& segment : sFuseSegments) {
        Math_Vec3f_Copy(&segment.pos, bombPos);
        Math_Vec3f_Copy(&segment.velocity, &sZeroVec);
        segment.rotY = 0;
        segment.rotX = 0x4000;
    }
}

void UpdateKegFuseSegments(PlayState* play, Vec3f* fuseRootPos, Vec3f* fuseTip, Vec3f* kegCenter, s32 timer) {
    Vec3f diff;
    Vec3f step;
    PowderKegFuseSegment* segment = &sFuseSegments[0];
    PowderKegFuseSegment* next = &sFuseSegments[1];
    const f32 kegRadius = Math_Vec3f_DistXYZ(kegCenter, fuseRootPos);

    Math_Vec3f_Copy(&segment->pos, fuseRootPos);
    Math_Vec3f_Diff(fuseTip, fuseRootPos, &diff);
    segment->rotY = Math_Atan2S(diff.z, diff.x);
    segment->rotX = Math_Atan2S(sqrtf(SQ(diff.x) + SQ(diff.z)), diff.y);

    const s32 count = (timer / 240) + 1;
    for (s32 i = 0; i < count; i++, segment++, next++, timer -= 240) {
        const f32 length = (timer >= 240) ? 8.0f : (timer % 240) * (1.0f / 240) * 8.0f;
        CollisionPoly* poly;
        s32 bgId;
        Vec3f probe;

        Math_Vec3f_Sum(&next->pos, &next->velocity, &next->pos);
        f32 dist = Math_Vec3f_DistXYZAndStoreDiff(kegCenter, &next->pos, &diff);
        if (dist < kegRadius) {
            if (dist == 0.0f) {
                diff.x = 0.0f;
                diff.y = kegRadius;
                diff.z = 0.0f;
            } else {
                dist = kegRadius / dist;
                diff.x *= dist;
                diff.y *= dist;
                diff.z *= dist;
            }
            Math_Vec3f_Sum(kegCenter, &diff, &next->pos);
        }

        if (Math_Vec3f_DistXYZAndStoreDiff(&segment->pos, &next->pos, &diff) == 0.0f) {
            diff.x = 0.0f;
            diff.y = length;
            diff.z = 0.0f;
        }

        next->rotY = Math_Atan2S(diff.z, diff.x);
        next->rotX = Math_Atan2S(sqrtf(SQ(diff.x) + SQ(diff.z)), diff.y);
        next->rotY = static_cast<s16>(CLAMP(BinangSub(next->rotY, segment->rotY), -8000, 8000)) + segment->rotY;
        next->rotX = static_cast<s16>(CLAMP(BinangSub(next->rotX, segment->rotX), -8000, 8000)) + segment->rotX;

        f32 horizontal = Math_CosS(next->rotX) * length;
        step.x = Math_SinS(next->rotY) * horizontal;
        step.z = Math_CosS(next->rotY) * horizontal;
        step.y = Math_SinS(next->rotX) * length;

        Math_Vec3f_Sum(&segment->pos, &step, &next->pos);
        Math_Vec3f_Copy(&probe, &next->pos);
        probe.y += 50.0f;

        f32 rise = BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &bgId, &probe) - next->pos.y;
        if (rise >= 0.0f) {
            step.y += rise;
            if (length < step.y) {
                step.y = length;
                rise = 0.0f;
            } else {
                rise = sqrtf(SQ(length) - SQ(step.y));
            }

            if (horizontal == 0.0f) {
                step.x = rise;
            } else {
                rise /= horizontal;
                step.x *= rise;
                step.z *= rise;
            }

            Math_Vec3f_Sum(&segment->pos, &step, &next->pos);
            Math_Vec3f_Copy(&next->velocity, &sZeroVec);
        }

        next->velocity.y += -1.0f;
        if (next->velocity.y < -10.0f) {
            next->velocity.y = -10.0f;
        }
    }

    Math_Vec3f_Copy(fuseRootPos, &sFuseSegments[0].pos);
}

void Move(OotmmBomMm* self, PlayState* play) {
    if (Actor_HasParent(&self->actor, play)) {
        self->actionFunc = WaitForRelease;
        self->actor.room = -1;
        return;
    }

    if ((self->actor.velocity.y > 0.0f) && (self->actor.bgCheckFlags & kBgCheckCeiling)) {
        self->actor.velocity.y = -self->actor.velocity.y;
    }

    if ((self->actor.speedXZ != 0.0f) && (self->actor.bgCheckFlags & kBgCheckWall)) {
        s16 yDiff = BinangSub(self->actor.wallYaw, self->actor.world.rot.y);
        if (ABS(yDiff) > 0x4000) {
            self->actor.world.rot.y =
                BinangSub(self->actor.wallYaw - self->actor.world.rot.y + self->actor.wallYaw, 0x8000);
        }
        Audio_PlayActorSound2(&self->actor, self->isPowderKeg ? NA_SE_EV_PUT_DOWN_WOODBOX : NA_SE_EV_BOMB_BOUND);
        Actor_MoveXZGravity(&self->actor);
        self->actor.speedXZ *= 0.7f;
        self->actor.bgCheckFlags &= ~kBgCheckWall;
    }

    if (!(self->actor.bgCheckFlags & kBgCheckGround)) {
        Math_StepToF(&self->actor.speedXZ, 0.0f, 0.08f);
    } else {
        const BombPhysics* physics = &sBombPhysics[self->isPowderKeg];
        const u32 floorType = func_80041D4C(&play->colCtx, self->actor.floorPoly, self->actor.floorBgId);
        Vec3f slopeNormal;

        if (floorType == kFloorTypeSand) {
            Math_ApproachF(&self->actor.shape.yOffset, 0.0f, 0.1f, 50.0f);
            s16 alpha = self->actor.shape.shadowAlpha;
            Math_ApproachS(&alpha, 0, 1, 20);
            self->actor.shape.shadowAlpha = static_cast<u8>(alpha);
        } else {
            Math_ApproachF(&self->actor.shape.yOffset, 700.0f, 1.0f, 700.0f);
        }

        f32 velX = Math_SinS(self->actor.world.rot.y) * self->actor.speedXZ;
        f32 velZ = Math_CosS(self->actor.world.rot.y) * self->actor.speedXZ;
        slopeNormal.x = COLPOLY_GET_NORMAL(self->actor.floorPoly->normal.x);
        slopeNormal.y = COLPOLY_GET_NORMAL(self->actor.floorPoly->normal.y);
        slopeNormal.z = COLPOLY_GET_NORMAL(self->actor.floorPoly->normal.z);

        velX += 3.0f * slopeNormal.x;
        velZ += 3.0f * slopeNormal.z;
        const f32 speed = sqrtf(SQ(velX) + SQ(velZ));

        if (speed < self->actor.speedXZ) {
            self->actor.speedXZ = (speed > 16.0f) ? 16.0f : speed;
            self->actor.world.rot.y = Math_Atan2S(velZ, velX);
        }

        if (!Math_StepToF(&self->actor.speedXZ, 0.0f, physics->deceleration)) {
            s16 target = self->actor.world.rot.y;
            if (ABS(BinangSub(self->actor.world.rot.y, self->actor.shape.rot.y)) > 0x4000) {
                target = static_cast<s16>(target + 0x8000);
            }
            Math_ScaledStepToS(&self->actor.shape.rot.y, target, static_cast<s16>(self->actor.speedXZ * 100.0f));
            self->rollAngle += static_cast<s16>(self->actor.speedXZ * 800.0f);
        }

        if (self->actor.bgCheckFlags & kBgCheckGroundTouch) {
            Audio_PlayActorSound2(&self->actor, self->isPowderKeg ? NA_SE_EV_PUT_DOWN_WOODBOX : NA_SE_EV_BOMB_BOUND);
            if (self->actor.velocity.y < physics->bounceThreshold) {
                if ((floorType == kFloorTypeSand) || (floorType == kFloorType14) || (floorType == kFloorType15)) {
                    self->actor.velocity.y = 0.0f;
                } else {
                    self->actor.velocity.y *= physics->bounceAmount;
                }
                self->actor.bgCheckFlags &= ~kBgCheckGround;
            }
        } else if (self->timer >= 4) {
            Actor_OfferCarry(&self->actor, play);
        }
    }

    Actor_MoveXZGravity(&self->actor);
}

void WaitForRelease(OotmmBomMm* self, PlayState* play) {
    if (Actor_HasNoParent(&self->actor, play)) {
        self->actionFunc = Move;
        self->actor.room = play->roomCtx.curRoom.num;
        self->actor.flags &= ~ACTOR_FLAG_UPDATE_DURING_OCARINA;
        self->actor.bgCheckFlags &= ~kBgCheckGround;
        Vec3sToVec3f(&self->actor.prevPos, &self->actor.home.rot);
        if (self->isPowderKeg) {
            OotmmCustomItems_SetKegFuse(0);
        }
        Move(self, play);
    } else {
        Vec3fToVec3s(&self->actor.home.rot, &self->actor.parent->world.pos);
        if (self->isPowderKeg) {
            OotmmCustomItems_SetKegFuse(self->timer);
        }
    }
    Math_ScaledStepToS(&self->rollAngle, 0, 0x7D0);
}

void Explode(OotmmBomMm* self, PlayState* play) {
    if (self->collider2.elements[0].dim.modelSphere.radius == 0) {
        self->actor.flags |= ACTOR_FLAG_DRAW_CULLING_DISABLED;
        func_800AA000(self->actor.xzDistToPlayer, 255, 20, 150);
    }

    self->collider2.elements[0].dim.worldSphere.radius = sExplosionRadius[self->isPowderKeg];
    if (self->timer == 7) {
        self->collider2.base.atFlags &= ~AT_TYPE_ENEMY;
    }

    if (self->actor.params == kBombTypeExplosion) {
        CollisionCheck_SetAT(play, &play->colChkCtx, &self->collider2.base);
    }

    for (s32 i = 0; i < 3; i++) {
        if (play->envCtx.adjLight1Color[i] != 0) {
            play->envCtx.adjLight1Color[i] -= 25;
        }
    }
    for (s32 i = 0; i < 3; i++) {
        if (play->envCtx.adjAmbientColor[i] != 0) {
            play->envCtx.adjAmbientColor[i] -= 25;
        }
    }

    if (self->timer == 0) {
        ReleaseFromPlayer(play, &self->actor);
        Actor_Kill(&self->actor);
    }

    if ((self->timer & 1) == 0) {
        const f32 phase = Rand_ZeroFloat(kPi);
        for (s32 i = 0; i < 15; i++) {
            Vec3f offset;
            Vec3f probe;
            CollisionPoly* poly;
            s32 bgId;

            Matrix_RotateY(((2.0f * (i * kPi)) / 15.0f) + phase, MTXMODE_NEW);
            MultVecZ((10 - self->timer) * 300.0f * 0.1f, &offset);
            probe.x = self->actor.world.pos.x + offset.x;
            probe.y = self->actor.world.pos.y + 500.0f;
            probe.z = self->actor.world.pos.z + offset.z;
            if (BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &bgId, &probe) != BGCHECK_Y_MIN) {
                const u32 floorType = func_80041D4C(&play->colCtx, poly, bgId);
                f32 floorY = BgCheck_EntityRaycastFloor1(&play->colCtx, &poly, &probe);

                if (floorType == kFloorTypeSand) {
                    Color_RGBA8 prim = sSandDustColor;
                    Color_RGBA8 env = sSandDustColor;
                    Vec3f velocity;
                    Vec3f accel;
                    Vec3f pos;

                    MultVecZ(5.0f, &velocity);
                    accel.x = accel.z = 0.0f;
                    velocity.y = 2.0f;
                    accel.y = 0.2f;
                    pos.x = probe.x;
                    pos.y = floorY;
                    pos.z = probe.z;

                    floorY = Rand_ZeroFloat(250.0f);
                    func_8002843C(play, &pos, &velocity, &accel, &prim, &env, static_cast<s16>(floorY + 950.0f), 10,
                                  static_cast<s16>(Rand_ZeroFloat(5.0f) + 14.0f));
                }
            }
        }
    }
}

} // namespace

static void DrawKegFuseSegments(PlayState* play, s32 fuseTimer) {
    PowderKegFuseSegment* segment = &sFuseSegments[0];

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Translate(segment->pos.x, segment->pos.y, segment->pos.z, MTXMODE_NEW);
    Matrix_RotateZYX(segment->rotX, segment->rotY, 0, MTXMODE_APPLY);
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kKegFuseMaterialDL);

    const s32 count = (fuseTimer / 240) + 1;
    segment = &sFuseSegments[1];
    for (s32 i = 1; i < count; i++, segment++) {
        Matrix_Translate(segment->pos.x, segment->pos.y, segment->pos.z, MTXMODE_NEW);
        Matrix_RotateZYX(segment->rotX, segment->rotY, 0, MTXMODE_APPLY);
        Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)(((i % 2) == 0) ? kKegFuseModel1DL : kKegFuseModel2DL));
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" int OotmmBomMm_IsCustomBomb(Actor* actor) {
    return (actor != nullptr) && (gOotmmBomMmId >= 0) && (actor->id == gOotmmBomMmId) ? 1 : 0;
}

extern "C" int OotmmBomMm_IsPowderKeg(Actor* actor) {
    return OotmmBomMm_IsCustomBomb(actor) && reinterpret_cast<OotmmBomMm*>(actor)->isPowderKeg ? 1 : 0;
}

extern "C" void OotmmBomMm_Init(Actor* thisx, PlayState* play) {
    OotmmBomMm* self = reinterpret_cast<OotmmBomMm*>(thisx);

    Actor_ProcessInitChain(&self->actor, sInitChain);
    ActorShape_Init(&self->actor.shape, 700.0f, ActorShadow_DrawCircle, 16.0f);
    self->actor.colChkInfo.mass = 200;
    self->actor.colChkInfo.cylRadius = 5;
    self->actor.colChkInfo.cylHeight = 10;

    self->flashSpeedScale = 7;
    const s16 bombType = self->actor.shape.rot.x & 3;
    self->isPowderKeg = (bombType == OOTMM_BOM_MM_TYPE_POWDER_KEG) ? 1 : 0;
    self->timer = self->isPowderKeg ? OotmmCustomItems_KegFuse() : 70;

    Collider_InitCylinder(play, &self->collider1);
    Collider_InitJntSph(play, &self->collider2);
    Collider_SetCylinder(play, &self->collider1, &self->actor, &sCylinderInit);

    if (!self->isPowderKeg) {
        Collider_SetJntSph(play, &self->collider2, &self->actor, &sBombSphereInit, self->collider2Elements);
        self->collider1.dim.radius = 6;
        self->collider1.dim.height = 11;
    } else {
        Collider_SetJntSph(play, &self->collider2, &self->actor, &sKegSphereInit, self->collider2Elements);
        self->collider1.dim.radius = 20;
        self->collider1.dim.height = 36;
        InitKegFuseSegments(&self->actor.world.pos);
    }

    self->collider2Elements[0].info.toucher.damage += (self->actor.shape.rot.z & 0xFF00) >> 8;
    self->actor.shape.rot.z &= 0xFF;
    if (self->actor.shape.rot.z & 0x80) {
        self->actor.shape.rot.z |= 0xFF00;
    }

    self->collider2.elements[0].dim.worldSphere.center.x = static_cast<s16>(self->actor.world.pos.x);
    self->collider2.elements[0].dim.worldSphere.center.y = static_cast<s16>(self->actor.world.pos.y);
    self->collider2.elements[0].dim.worldSphere.center.z = static_cast<s16>(self->actor.world.pos.z);

    self->actor.flags |= ACTOR_FLAG_UPDATE_DURING_OCARINA;

    if (bombType != OOTMM_BOM_MM_TYPE_BLAST) {
        self->actionFunc = WaitForRelease;
        self->actor.room = -1;
        Actor_SetScale(&self->actor, sBombScales[self->isPowderKeg]);
    } else {
        self->actionFunc = Move;
        OotmmCustomItems_SetKegFuse(0);
    }
}

extern "C" void OotmmBomMm_Destroy(Actor* thisx, PlayState* play) {
    OotmmBomMm* self = reinterpret_cast<OotmmBomMm*>(thisx);
    Collider_DestroyJntSph(play, &self->collider2);
    Collider_DestroyCylinder(play, &self->collider1);
}

extern "C" void OotmmBomMm_Update(Actor* thisx, PlayState* play) {
    Vec3f effVelocity = { 0.0f, 0.0f, 0.0f };
    Vec3f effAccel = { 0.0f, 0.0f, 0.0f };
    Vec3f effPos;
    Vec3f dustAccel = { 0.0f, 0.6f, 0.0f };
    Color_RGBA8 dustColor = { 255, 255, 255, 255 };
    OotmmBomMm* self = reinterpret_cast<OotmmBomMm*>(thisx);
    Player* player = GET_PLAYER(play);

    if (player->stateFlags1 & PLAYER_STATE1_HOOKSHOT_FALLING) {
        return;
    }

    if (self->sinkTimer != 0) {
        self->sinkTimer--;
        Math_ApproachZeroF(&thisx->speedXZ, 1.0f, 1.0f);
        Actor_MoveXZGravity(thisx);
        Actor_UpdateBgCheckInfo(play, thisx, 35.0f, 10.0f, 36.0f, 4);
        if (self->sinkTimer == 0) {
            if (self->isPowderKeg) {
                OotmmCustomItems_SetKegFuse(0);
            }
            Actor_Kill(thisx);
        }
        return;
    }

    thisx->gravity = -1.2f;
    if (self->timer != 0) {
        if (!self->isPowderKeg || (self->actionFunc == Explode) || !Play_InCsMode(play)) {
            self->timer--;
        }
    }

    if ((!self->isPowderKeg && (self->timer == 67)) || (self->isPowderKeg && (self->timer <= 2400))) {
        Audio_PlayActorSound2(thisx, NA_SE_PL_TAKE_OUT_SHIELD);
        Actor_SetScale(thisx, sBombScales[self->isPowderKeg]);
    }

    if ((thisx->xzDistToPlayer >= 20.0f) || (fabsf(thisx->yDistToPlayer) >= 80.0f)) {
        self->bumpOn = 1;
    }

    self->actionFunc(self, play);

    Actor_UpdateBgCheckInfo(play, thisx, 35.0f, 10.0f, 36.0f, 0x1F);

    if (thisx->params == kBombTypeBody) {
        const FlashTimings* flash = &sFlashTimings[self->isPowderKeg];

        dustAccel.y = 0.2f;
        Math_Vec3f_Copy(&effPos, &thisx->home.pos);
        if ((play->gameplayFrames % 2) == 0) {
            EffectSsGSpk_SpawnFuse(play, thisx, &effPos, &effVelocity, &effAccel);
        }
        if (self->isPowderKeg) {
            PlaySfxWithChannelIO(&thisx->projectedPos, NA_SE_IT_BOMB_IGNIT - SFX_FLAG,
                                 (self->flashSpeedScale == 7) ? 0 : (self->flashSpeedScale == 3) ? 1 : 2);
        } else {
            Audio_PlayActorSound2(thisx, NA_SE_IT_BOMB_IGNIT - SFX_FLAG);
        }

        effPos.y += 3.0f;
        func_8002829C(play, &effPos, &effVelocity, &dustAccel, &dustColor, &dustColor, 50, 5);

        if ((self->collider1.base.acFlags & AC_HIT) ||
            ((self->collider1.base.ocFlags1 & OC1_HIT) && (self->collider1.base.oc != nullptr) &&
             ((self->collider1.base.oc->category == ACTORCAT_ENEMY) ||
              (self->collider1.base.oc->category == ACTORCAT_BOSS)))) {
            self->timer = 0;
            thisx->shape.rot.z = 0;
        } else if ((self->timer > 100) && Player_IsBurningStickInRange(play, &thisx->world.pos, 30.0f, 50.0f)) {
            self->timer = 100;
        }

        dustAccel.y = 0.2f;
        effPos = thisx->world.pos;
        effPos.y += 10.0f;

        if ((self->timer == flash->first) || (self->timer == flash->second) || (self->timer == 3)) {
            thisx->shape.rot.z = 0;
            self->flashSpeedScale >>= 1;
        }

        if ((self->timer < flash->start) && (self->timer & (self->flashSpeedScale + 1))) {
            Math_ApproachF(&self->flashIntensity, 140.0f, 1.0f, 140.0f / self->flashSpeedScale);
        } else {
            Math_ApproachZeroF(&self->flashIntensity, 1.0f, 140.0f / self->flashSpeedScale);
        }

        if (self->timer < 3) {
            Actor_SetScale(thisx, thisx->scale.x + 0.002f);
        }

        if (self->timer == 0) {
            effPos = thisx->world.pos;
            effPos.y += 10.0f;
            if (Actor_HasParent(thisx, play)) {
                effPos.y += 30.0f;
            }
            OotmmClearTag_Spawn(play, effPos.x, effPos.y - 10.0f, effPos.z, 0, self->isPowderKeg);
            func_80033E88(thisx, play, sQuakeY[self->isPowderKeg], sQuakeDurations[self->isPowderKeg]);
            play->envCtx.adjLight1Color[0] = play->envCtx.adjLight1Color[1] = play->envCtx.adjLight1Color[2] = 250;
            play->envCtx.adjAmbientColor[0] = play->envCtx.adjAmbientColor[1] = play->envCtx.adjAmbientColor[2] = 250;
            Camera_AddQuake(&play->mainCamera, 2, 11, 8);
            thisx->params = kBombTypeExplosion;
            self->timer = 10;
            thisx->flags |= (ACTOR_FLAG_DRAW_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA);
            self->actionFunc = Explode;
            if (self->isPowderKeg) {
                OotmmCustomItems_SetKegFuse(0);
            }
            Audio_PlayActorSound2(thisx, NA_SE_IT_BOMB_EXPLOSION);
        }
    }

    Actor_SetFocus(thisx, 20.0f);

    if (thisx->params <= kBombTypeBody) {
        Collider_UpdateCylinder(thisx, &self->collider1);
        if (!Actor_HasParent(thisx, play) && (self->bumpOn != 0)) {
            CollisionCheck_SetOC(play, &play->colChkCtx, &self->collider1.base);
        }
        CollisionCheck_SetAC(play, &play->colChkCtx, &self->collider1.base);
    }

    if ((sBombScales[self->isPowderKeg] <= thisx->scale.x) && (thisx->params != kBombTypeExplosion)) {
        if (thisx->yDistToWater >= 20.0f) {
            Vec3f waterPos;
            waterPos.x = thisx->world.pos.x;
            waterPos.y = thisx->world.pos.y + thisx->yDistToWater;
            waterPos.z = thisx->world.pos.z;
            EffectSsGRipple_Spawn(play, &waterPos, 70, 500, 0);
            EffectSsGRipple_Spawn(play, &waterPos, 70, 500, 10);
            waterPos.y += 10.0f;
            EffectSsGSplash_Spawn(play, &waterPos, nullptr, nullptr, 1, 500);
            OotmmClearTag_Spawn(play, waterPos.x, waterPos.y, waterPos.z, 1, OOTMM_CLEAR_TAG_SMOKE);
            SoundSource_PlaySfxAtFixedWorldPos(play, &thisx->world.pos, 30, NA_SE_IT_BOMB_UNEXPLOSION);
            self->flashIntensity = 0.0f;
            thisx->velocity.y = (KREG(83) * 0.1f) + -2.0f;
            thisx->gravity = (KREG(84) * 0.1f) + -0.5f;
            self->sinkTimer = static_cast<u8>(KREG(81) + 10);
        } else if (thisx->bgCheckFlags & kBgCheckWaterTouch) {
            thisx->bgCheckFlags &= ~kBgCheckWaterTouch;
            Audio_PlayActorSound2(thisx, NA_SE_EV_BOMB_DROP_WATER);
        }
    }
}

extern "C" void OotmmBomMm_Draw(Actor* thisx, PlayState* play) {
    OotmmBomMm* self = reinterpret_cast<OotmmBomMm*>(thisx);

    OPEN_DISPS(play->state.gfxCtx);

    if (self->actor.params == kBombTypeBody) {
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        Collider_UpdateSpheres(0, &self->collider2);

        if (!self->isPowderKeg) {
            func_8002EBCC(&self->actor, play, 0);
            Matrix_MultVec3f(&sBombCapOffset, &self->actor.home.pos);
            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gBombCapDL);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            RotateXS(0x4000, MTXMODE_APPLY);
            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPPipeSync(POLY_OPA_DISP++);
            gDPSetEnvColor(POLY_OPA_DISP++, (s8)self->flashIntensity, 0, 40, 255);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, (s8)self->flashIntensity, 0, 40, 255);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gBombBodyDL);
        } else {
            Vec3f fuseTip;
            Vec3f kegCenter;

            if (self->rollAngle != 0) {
                const s16 yawDiff = self->actor.world.rot.y - self->actor.shape.rot.y;
                const f32 lift =
                    (1000.0f / Math_CosS(ABS(static_cast<s16>(self->rollAngle % 10922)) - 0x1555)) + -1000.0f;

                RotateYS(yawDiff, MTXMODE_APPLY);
                Matrix_Translate(0.0f, lift, 0.0f, MTXMODE_APPLY);
                RotateXS(self->rollAngle, MTXMODE_APPLY);
                RotateYS(-yawDiff, MTXMODE_APPLY);
            }

            Matrix_MultVec3f(&sFuseRootOffset, &self->actor.home.pos);
            Matrix_MultVec3f(&sFuseTipOffset, &fuseTip);
            Matrix_MultVec3f(&sZeroVec, &kegCenter);

            gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
            gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kKegBarrelDL);
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kKegSkullDL);

            UpdateKegFuseSegments(play, &self->actor.home.pos, &fuseTip, &kegCenter, self->timer);
            DrawKegFuseSegments(play, self->timer);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}
