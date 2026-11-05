#include <math.h>

#include "global.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "objects/gameplay_keep/gameplay_keep.h"
}

#include "soh/frame_interpolation.h"
#include "OotmmClearTag.h"

extern "C" int16_t gOotmmClearTagId = -1;

namespace {

enum EffectType : u8 {
    EFFECT_AVAILABLE,
    EFFECT_DEBRIS,
    EFFECT_FIRE,
    EFFECT_SMOKE,
    EFFECT_FLASH,
    EFFECT_LIGHT_RAYS,
    EFFECT_SHOCKWAVE,
    EFFECT_SPLASH,
    EFFECT_ISOLATED_SMOKE,
};

constexpr f32 kPi = static_cast<f32>(M_PI);
constexpr u16 kBgCheckFloor = 4;
constexpr u16 kBgCheckWater = 0x20;

constexpr const char* kDebrisMaterialDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagDebrisEffectMaterialDL";
constexpr const char* kDebrisDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagDebrisEffectDL";
constexpr const char* kFireMaterialDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagFireEffectMaterialDL";
constexpr const char* kFireDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagFireEffectDL";
constexpr const char* kFlashDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagFlashEffectDL";
constexpr const char* kFlashGroundDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagFlashEffectGroundDL";
constexpr const char* kLightRayMaterialDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagLightRayEffectMaterialDL";
constexpr const char* kLightRayDL = "__OTR__overlays/mm_ovl_En_Clear_Tag/gClearTagLightRayEffectDL";

Vec3f sZeroVector = { 0.0f, 0.0f, 0.0f };

const Vec3f sLightRayEnvColor[] = {
    { 255.0f, 255.0f, 0.0f },
    { 255.0f, 100.0f, 100.0f },
    { 100.0f, 255.0f, 100.0f },
    { 100.0f, 100.0f, 255.0f },
};

const Vec3f sLightRayPrimColor[] = {
    { 255.0f, 255.0f, 255.0f },
    { 255.0f, 255.0f, 255.0f },
    { 255.0f, 255.0f, 255.0f },
    { 255.0f, 255.0f, 255.0f },
};

const f32 sFlashMaxScale[] = { 6.0f, 12.0f, 9.0f };
const f32 sSmokeScale[] = { 3.0f, 6.0f };
const f32 sShockwaveMaxScale[] = { 15.0f, 30.0f, 20.0f };
const f32 sDebrisScale[] = { 0.03f, 0.06f, 0.04f };
const f32 sLightRayScale[] = { 1000.0f, 2000.0f, 1500.0f, 800.0f, 1300.0f };
const f32 sLightRayMaxScaleTarget[] = { 15.0f, 30.0f, 20.0f, 10.0f, 20.0f };
const f32 sLightRayMaxScale[] = { 25.0f, 100.0f, 48.0f, 20.0f, 32.0f };

const void* const sWaterSplashTextures[] = {
    gEffWaterSplash1Tex, gEffWaterSplash2Tex, gEffWaterSplash3Tex, gEffWaterSplash4Tex,
    gEffWaterSplash5Tex, gEffWaterSplash6Tex, gEffWaterSplash7Tex, gEffWaterSplash8Tex,
    nullptr,             nullptr,             nullptr,
};

void MultVecZ(f32 z, Vec3f* dest) {
    Vec3f src = { 0.0f, 0.0f, z };
    Matrix_MultVec3f(&src, dest);
}

f32 PitchOf(const Vec3f& velocity) {
    return -Math_Atan2F(sqrtf(SQ(velocity.x) + SQ(velocity.z)), velocity.y);
}

OotmmClearTagEffect* FindAvailable(OotmmClearTag* self, s32 limit) {
    OotmmClearTagEffect* effect = self->effect;
    for (s32 i = 0; i < limit; i++, effect++) {
        if (effect->type == EFFECT_AVAILABLE) {
            return effect;
        }
    }
    return nullptr;
}

void CreateDebrisEffect(OotmmClearTag* self, Vec3f* pos, Vec3f* velocity, Vec3f* accel, f32 scale, f32 rotationZ) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT - 1);
    if (effect == nullptr) {
        return;
    }
    effect->type = EFFECT_DEBRIS;
    effect->pos = *pos;
    effect->velocity = *velocity;
    effect->accel = *accel;
    effect->scale = scale;
    effect->rotationY = Rand_ZeroFloat(kPi * 2);
    effect->rotationX = Rand_ZeroFloat(kPi * 2);
    effect->effectsTimer = effect->bounces = 0;
    effect->rotationZ = rotationZ;
    effect->actionTimer = static_cast<u8>(Rand_ZeroFloat(10.0f));
}

void CreateSmokeEffect(OotmmClearTag* self, Vec3f* pos, f32 scale) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT);
    if (effect == nullptr) {
        return;
    }
    effect->actionTimer = static_cast<u8>(Rand_ZeroFloat(100.0f));
    effect->type = EFFECT_SMOKE;
    effect->pos = *pos;
    effect->velocity = sZeroVector;
    effect->accel = sZeroVector;
    effect->scale = scale;
    effect->maxScale = 2.0f * scale;
    effect->smokeScaleX = 1.0f;
    effect->smokeScaleY = 1.0f;
    effect->primColor.a = 255.0f;
    effect->envColor.r = 255.0f;
    effect->envColor.b = 255.0f;
    effect->primColor.r = 200.0f;
    effect->primColor.g = 20.0f;
    effect->primColor.b = 0.0f;
    effect->envColor.g = 215.0f;
}

void CreateIsolatedSmokeEffect(OotmmClearTag* self, Vec3f* pos, f32 scale) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT);
    if (effect == nullptr) {
        return;
    }
    effect->actionTimer = static_cast<u8>(Rand_ZeroFloat(100.0f));
    effect->type = EFFECT_ISOLATED_SMOKE;
    effect->pos = *pos;
    effect->velocity = sZeroVector;
    effect->accel = sZeroVector;
    effect->scale = scale;
    effect->maxScale = 2.0f * scale;
    effect->smokeScaleX = 1.0f;
    effect->smokeScaleY = 1.0f;
    if (scale <= 1.1f) {
        effect->scale = ((KREG(23) * 0.1f) + 1.0f) * scale;
        effect->primColor.a = KREG(16) + 150.0f;
        effect->primColor.r = KREG(17);
        effect->primColor.g = KREG(18);
        effect->primColor.b = KREG(19);
        effect->envColor.r = KREG(20);
        effect->envColor.g = KREG(21);
        effect->envColor.b = KREG(22);
    } else {
        effect->envColor.r = effect->envColor.g = effect->envColor.b = 0.0f;
        effect->primColor.r = effect->primColor.g = effect->primColor.b = 0.0f;
        effect->primColor.a = 255.0f;
    }
}

void CreateFlashEffect(OotmmClearTag* self, Vec3f* pos, f32 scale, f32 floorHeight) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT - 1);
    if (effect == nullptr) {
        return;
    }
    effect->type = EFFECT_FLASH;
    effect->pos = *pos;
    effect->velocity = sZeroVector;
    effect->accel = sZeroVector;
    effect->scale = 0.0f;
    effect->maxScale = scale * 3.0f;
    effect->rotationZ = floorHeight;
    effect->primColor.a = 255.0f;
}

void CreateLightRayEffect(OotmmClearTag* self, Vec3f* pos, Vec3f* velocity, Vec3f* accel, f32 scale,
                          f32 maxScaleTarget, s16 colorIndex, s16 alphaDecrementSpeed) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT - 1);
    if (effect == nullptr) {
        return;
    }
    effect->type = EFFECT_LIGHT_RAYS;
    effect->pos = *pos;
    effect->velocity = *velocity;
    effect->accel = *accel;
    effect->scale = scale / 1000.0f;
    effect->maxScale = 1.0f;
    effect->maxScaleTarget = maxScaleTarget;
    effect->lightRayAlpha = static_cast<s16>(Rand_ZeroFloat(100.0f) + 200.0f);
    effect->lightRayAlphaDecrementSpeed = alphaDecrementSpeed;
    effect->actionTimer = static_cast<u8>(Rand_ZeroFloat(10.0f));
    effect->rotationY = Math_Atan2F(effect->velocity.z, effect->velocity.x);
    effect->rotationX = PitchOf(effect->velocity);
    effect->envColor.r = sLightRayEnvColor[colorIndex].x;
    effect->envColor.g = sLightRayEnvColor[colorIndex].y;
    effect->envColor.b = sLightRayEnvColor[colorIndex].z;
    effect->primColor.r = sLightRayPrimColor[colorIndex].x;
    effect->primColor.g = sLightRayPrimColor[colorIndex].y;
    effect->primColor.b = sLightRayPrimColor[colorIndex].z;
}

void CreateShockwaveEffect(OotmmClearTag* self, Vec3f* pos, f32 maxScale, s16 actionTimer) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT - 1);
    if (effect == nullptr) {
        return;
    }
    effect->type = EFFECT_SHOCKWAVE;
    effect->pos = *pos;
    effect->velocity = sZeroVector;
    effect->accel = sZeroVector;
    effect->maxScale = maxScale;
    effect->scale = 0.0f;
    effect->actionTimer = static_cast<u8>(actionTimer);
    effect->primColor.a = 200.0f;
}

void CreateSplashEffect(OotmmClearTag* self, Vec3f* pos, s16 effectsTimer) {
    OotmmClearTagEffect* effect = FindAvailable(self, OOTMM_CLEAR_TAG_EFFECT_COUNT - 1);
    if (effect == nullptr) {
        return;
    }
    effect->type = EFFECT_SPLASH;
    effect->pos = *pos;
    effect->velocity = sZeroVector;
    effect->accel = sZeroVector;
    effect->scale = 0.0f;
    effect->maxScale = 0.0f;
    effect->actionTimer = 0;
    effect->effectsTimer = effectsTimer;
    effect->rotationX = 0.78f;
}

void SpawnLightRays(OotmmClearTag* self, Vec3f* pos, s16 type, s32 count, bool isolated) {
    Vec3f velocity;
    Vec3f accel;
    for (s32 i = 0; i < count; i++) {
        f32 maxScale = sLightRayMaxScale[type] + Rand_ZeroFloat(sLightRayMaxScale[type]);
        Matrix_RotateY(Rand_ZeroFloat(kPi * 2), MTXMODE_NEW);
        Matrix_RotateX(Rand_ZeroFloat(kPi * 2), MTXMODE_APPLY);
        MultVecZ(maxScale, &velocity);
        accel.x = velocity.x * -0.03f;
        accel.y = velocity.y * -0.03f;
        accel.z = velocity.z * -0.03f;
        CreateLightRayEffect(self, pos, &velocity, &accel,
                             sLightRayScale[type] + Rand_ZeroFloat(sLightRayScale[type] * 0.5f),
                             sLightRayMaxScaleTarget[type], isolated ? self->actor.world.rot.z : 0,
                             static_cast<s16>(Rand_ZeroFloat(10.0f) + 20.0f));
    }
}

void UpdateEffects(OotmmClearTag* self, PlayState* play) {
    OotmmClearTagEffect* effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if (effect->type == EFFECT_AVAILABLE) {
            continue;
        }
        effect->actionTimer++;

        effect->pos.x += effect->velocity.x;
        f32 originalY = effect->pos.y;
        effect->pos.y += effect->velocity.y;
        effect->pos.z += effect->velocity.z;
        effect->velocity.x += effect->accel.x;
        effect->velocity.y += effect->accel.y;
        effect->velocity.z += effect->accel.z;

        if (effect->type == EFFECT_DEBRIS) {
            if (effect->velocity.y < -5.0f) {
                effect->velocity.y = -5.0f;
            }
            if (effect->velocity.y < 0.0f) {
                Vec3f sphereCenter = effect->pos;
                sphereCenter.y += 5.0f;
                if (BgCheck_SphVsFirstPoly(&play->colCtx, &sphereCenter, 11.0f)) {
                    effect->pos.y = originalY;
                    if (effect->bounces <= 0) {
                        effect->bounces++;
                        effect->velocity.y *= -0.5f;
                        effect->effectsTimer = static_cast<s16>(Rand_ZeroFloat(20.0f) + 25.0f);
                    } else {
                        effect->velocity.x = effect->velocity.z = effect->accel.y = effect->velocity.y = 0.0f;
                    }
                }
            }
            if (effect->accel.y != 0.0f) {
                effect->rotationY += 0.5f;
                effect->rotationX += 0.35f;
            }
            if (effect->effectsTimer == 1) {
                effect->type = EFFECT_AVAILABLE;
            }
        } else if (effect->type == EFFECT_FIRE) {
            Math_ApproachZeroF(&effect->primColor.a, 1.0f, 15.0f);
            if (effect->primColor.a <= 0.0f) {
                effect->type = EFFECT_AVAILABLE;
            }
        } else if (effect->type == EFFECT_SHOCKWAVE) {
            if (effect->actionTimer > 3) {
                Math_ApproachF(&effect->scale, effect->maxScale, 0.2f, effect->maxScale * 0.6666666f);
                Math_ApproachZeroF(&effect->primColor.a, 1.0f, 15.0f);
                if (effect->primColor.a <= 0.0f) {
                    effect->type = EFFECT_AVAILABLE;
                }
            }
        } else if (effect->type == EFFECT_SMOKE) {
            Math_ApproachZeroF(&effect->primColor.r, 1.0f, 20.0f);
            Math_ApproachZeroF(&effect->primColor.g, 1.0f, 2.0f);
            Math_ApproachZeroF(&effect->envColor.r, 1.0f, 25.5f);
            Math_ApproachZeroF(&effect->envColor.g, 1.0f, 21.5f);
            Math_ApproachZeroF(&effect->envColor.b, 1.0f, 25.5f);
            Math_ApproachF(&effect->scale, effect->maxScale, 0.05f, 0.1f);
            if (effect->primColor.r == 0.0f) {
                Math_ApproachF(&effect->smokeScaleX, 3.0f, 0.1f, 0.01f);
                Math_ApproachF(&effect->smokeScaleY, 3.0f, 0.1f, 0.02f);
                Math_ApproachZeroF(&effect->primColor.a, 1.0f, 5.0f);
                if (effect->primColor.a <= 0.0f) {
                    effect->type = EFFECT_AVAILABLE;
                }
            }
        } else if (effect->type == EFFECT_ISOLATED_SMOKE) {
            Math_ApproachF(&effect->scale, effect->maxScale, 0.05f, 0.1f);
            if (effect->actionTimer > 10) {
                Math_ApproachF(&effect->smokeScaleX, 3.0f, 0.1f, 0.01f);
                Math_ApproachF(&effect->smokeScaleY, 3.0f, 0.1f, 0.02f);
                Math_ApproachZeroF(&effect->primColor.a, 1.0f, 5.0f);
                if (effect->primColor.a <= 0.0f) {
                    effect->type = EFFECT_AVAILABLE;
                }
            }
        } else if (effect->type == EFFECT_FLASH) {
            Math_ApproachF(&effect->scale, effect->maxScale, 0.5f, 6.0f);
            Math_ApproachZeroF(&effect->primColor.a, 1.0f, 15.0f);
            if (effect->primColor.a <= 0.0f) {
                effect->type = EFFECT_AVAILABLE;
            }
        } else if (effect->type == EFFECT_LIGHT_RAYS) {
            effect->rotationZ += Rand_ZeroFloat(kPi / 2) + (kPi / 2);
            effect->lightRayAlpha -= effect->lightRayAlphaDecrementSpeed;
            if (effect->lightRayAlpha <= 0) {
                effect->lightRayAlpha = 0;
                effect->type = EFFECT_AVAILABLE;
            }
            effect->primColor.a = effect->lightRayAlpha;
            if (effect->primColor.a > 255.0f) {
                effect->primColor.a = 255.0f;
            }
            Math_ApproachF(&effect->maxScale, effect->maxScaleTarget, 1.0f, (effect->maxScaleTarget / 15.0f) * 4.0f);
        } else if (effect->type == EFFECT_SPLASH) {
            if (effect->effectsTimer == 0) {
                effect->scale = 7.0f;
                Math_ApproachF(&effect->maxScale, 500.0f, 1.0f, 50.0f);
                Math_ApproachF(&effect->rotationX, 1.5f, 1.0f, 0.12f);
                if (effect->actionTimer > 7) {
                    effect->type = EFFECT_AVAILABLE;
                }
            } else {
                effect->actionTimer = 0;
            }
        }

        if (effect->effectsTimer != 0) {
            effect->effectsTimer--;
        }
    }
}

void LoadMatrix(Gfx** disp, GraphicsContext* gfxCtx) {
    gSPMatrix((*disp)++, MATRIX_NEWMTX(gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
}

} // namespace

extern "C" void OotmmClearTag_Destroy(Actor* thisx, PlayState* play) {
}

extern "C" void OotmmClearTag_Init(Actor* thisx, PlayState* play) {
    OotmmClearTag* self = reinterpret_cast<OotmmClearTag*>(thisx);
    Vec3f pos;
    Vec3f velocity;
    Vec3f accel;

    self->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    if (thisx->params < 0) {
        Actor_Kill(thisx);
        return;
    }

    self->activeTimer = 70;
    Math_Vec3f_Copy(&pos, &self->actor.world.pos);

    if (thisx->params == OOTMM_CLEAR_TAG_SMOKE) {
        CreateIsolatedSmokeEffect(self, &pos, self->actor.world.rot.z);
        return;
    }

    if (thisx->params != OOTMM_CLEAR_TAG_SPLASH) {
        if ((thisx->params == OOTMM_CLEAR_TAG_SMALL_LIGHT_RAYS) || (thisx->params == OOTMM_CLEAR_TAG_LARGE_LIGHT_RAYS)) {
            SpawnLightRays(self, &pos, thisx->params, 54, true);
            return;
        }

        if ((self->actor.world.rot.x != 0) || (self->actor.world.rot.y != 0) || (self->actor.world.rot.z != 0)) {
            self->flashEnvColor.r = static_cast<u8>(self->actor.world.rot.x);
            self->flashEnvColor.g = static_cast<u8>(self->actor.world.rot.y);
            self->flashEnvColor.b = static_cast<u8>(self->actor.world.rot.z);
        } else {
            self->flashEnvColor.r = 255;
            self->flashEnvColor.g = 0;
        }

        Actor_UpdateBgCheckInfo(play, &self->actor, 50.0f, 30.0f, 100.0f, kBgCheckFloor);
        pos = self->actor.world.pos;
        CreateFlashEffect(self, &pos, sFlashMaxScale[thisx->params], self->actor.floorHeight);

        if (!(self->actor.bgCheckFlags & kBgCheckWater)) {
            if (thisx->params < 10) {
                pos.y = self->actor.world.pos.y - 40.0f;
                if (thisx->params != OOTMM_CLEAR_TAG_POP) {
                    CreateSmokeEffect(self, &pos, sSmokeScale[thisx->params]);
                }
                pos.y = self->actor.floorHeight + 3.0f;
                CreateShockwaveEffect(self, &pos, sShockwaveMaxScale[thisx->params], 0);
                CreateShockwaveEffect(self, &pos, sShockwaveMaxScale[thisx->params], 3);
                if (thisx->params == OOTMM_CLEAR_TAG_LARGE_EXPLOSION) {
                    CreateShockwaveEffect(self, &pos, sShockwaveMaxScale[thisx->params], 6);
                }
            }

            if (thisx->params != OOTMM_CLEAR_TAG_POP) {
                pos.y = self->actor.world.pos.y;
                for (s32 i = 0; i < 18; i++) {
                    velocity.x = sinf(i * (33.0f / 40.0f)) * i * 0.5f;
                    velocity.z = cosf(i * (33.0f / 40.0f)) * i * 0.5f;
                    velocity.y = Rand_ZeroFloat(8.0f) + 7.0f;
                    velocity.x += Rand_CenteredFloat(0.5f);
                    velocity.z += Rand_CenteredFloat(0.5f);
                    accel.x = 0.0f;
                    accel.y = -1.0f;
                    accel.z = 0.0f;
                    CreateDebrisEffect(self, &pos, &velocity, &accel,
                                       sDebrisScale[thisx->params] + Rand_ZeroFloat(sDebrisScale[thisx->params]),
                                       self->actor.floorHeight);
                }
            }
        }

        pos = self->actor.world.pos;
        SpawnLightRays(self, &pos, thisx->params, 44, false);
    }

    CreateSplashEffect(self, &pos, 0);
    CreateSplashEffect(self, &pos, 2);
}

extern "C" void OotmmClearTag_Update(Actor* thisx, PlayState* play) {
    OotmmClearTag* self = reinterpret_cast<OotmmClearTag*>(thisx);
    if (self->activeTimer != 0) {
        self->activeTimer--;
    }
    if (self->activeTimer != 0) {
        UpdateEffects(self, play);
    } else {
        Actor_Kill(&self->actor);
    }
}

extern "C" void OotmmClearTag_Draw(Actor* thisx, PlayState* play) {
    OotmmClearTag* self = reinterpret_cast<OotmmClearTag*>(thisx);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OotmmClearTagEffect* effect;
    bool materialApplied = false;
    MtxF mtxF;

    OPEN_DISPS(gfxCtx);

    Gfx_SetupDL_25Opa(gfxCtx);
    Gfx_SetupDL_25Xlu(gfxCtx);

    effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if (effect->type != EFFECT_DEBRIS) {
            continue;
        }
        FrameInterpolation_RecordOpenChild(effect, effect->type);
        if (!materialApplied) {
            materialApplied = true;
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kDebrisMaterialDL);
        }
        Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
        Matrix_Scale(effect->scale, effect->scale, effect->scale, MTXMODE_APPLY);
        Matrix_RotateY(effect->rotationY, MTXMODE_APPLY);
        Matrix_RotateX(effect->rotationX, MTXMODE_APPLY);
        LoadMatrix(&POLY_OPA_DISP, gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)kDebrisDL);
        FrameInterpolation_RecordCloseChild();
    }

    if (self->actor.floorPoly != nullptr) {
        effect = self->effect;
        for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
            if (effect->type != EFFECT_SHOCKWAVE) {
                continue;
            }
            FrameInterpolation_RecordOpenChild(effect, effect->type);
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, (s8)effect->primColor.a);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, (s8)effect->primColor.a);
            func_80038A28(self->actor.floorPoly, effect->pos.x, effect->pos.y, effect->pos.z, &mtxF);
            Matrix_Put(&mtxF);
            Matrix_Scale(effect->scale, 1.0f, effect->scale, MTXMODE_APPLY);
            LoadMatrix(&POLY_XLU_DISP, gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gEffShockwaveDL);
            FrameInterpolation_RecordCloseChild();
        }

        materialApplied = false;
        effect = self->effect;
        for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
            if (effect->type != EFFECT_FLASH) {
                continue;
            }
            FrameInterpolation_RecordOpenChild(effect, effect->type);
            if (!materialApplied) {
                gDPPipeSync(POLY_XLU_DISP++);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 200, 0);
                materialApplied = true;
            }
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 200, (s8)(effect->primColor.a * 0.7f));
            func_80038A28(self->actor.floorPoly, effect->pos.x, self->actor.floorHeight, effect->pos.z, &mtxF);
            Matrix_Put(&mtxF);
            Matrix_Scale(effect->scale * 3.0f, 1.0f, effect->scale * 3.0f, MTXMODE_APPLY);
            LoadMatrix(&POLY_XLU_DISP, gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kFlashGroundDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    materialApplied = false;
    effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if ((effect->type != EFFECT_SMOKE) && (effect->type != EFFECT_ISOLATED_SMOKE)) {
            continue;
        }
        FrameInterpolation_RecordOpenChild(effect, effect->type);
        if (!materialApplied) {
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kFireMaterialDL);
            materialApplied = true;
        }
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetEnvColor(POLY_XLU_DISP++, (s8)effect->envColor.r, (s8)effect->envColor.g, (s8)effect->envColor.b, 128);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (s8)effect->primColor.r, (s8)effect->primColor.g,
                        (s8)effect->primColor.b, (s8)effect->primColor.a);
        gSPSegment(POLY_XLU_DISP++, 0x08,
                   reinterpret_cast<uintptr_t>(
                       Gfx_TwoTexScroll(gfxCtx, 0, 0, -effect->actionTimer * 5, 32, 64, 1, 0, 0, 32, 32)));
        Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(effect->smokeScaleX * effect->scale, effect->smokeScaleY * effect->scale, 1.0f, MTXMODE_APPLY);
        Matrix_Translate(0.0f, 20.0f, 0.0f, MTXMODE_APPLY);
        LoadMatrix(&POLY_XLU_DISP, gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kFireDL);
        FrameInterpolation_RecordCloseChild();
    }

    materialApplied = false;
    effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if (effect->type != EFFECT_FIRE) {
            continue;
        }
        FrameInterpolation_RecordOpenChild(effect, effect->type);
        if (!materialApplied) {
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kFireMaterialDL);
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 215, 255, 128);
            materialApplied = true;
        }
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 200, 20, 0, (s8)effect->primColor.a);
        gSPSegment(POLY_XLU_DISP++, 0x08,
                   reinterpret_cast<uintptr_t>(
                       Gfx_TwoTexScroll(gfxCtx, 0, 0, -effect->actionTimer * 15, 32, 64, 1, 0, 0, 32, 32)));
        Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(effect->scale, effect->scale, 1.0f, MTXMODE_APPLY);
        LoadMatrix(&POLY_XLU_DISP, gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kFireDL);
        FrameInterpolation_RecordCloseChild();
    }

    materialApplied = false;
    effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if (effect->type != EFFECT_FLASH) {
            continue;
        }
        FrameInterpolation_RecordOpenChild(effect, effect->type);
        if (!materialApplied) {
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetEnvColor(POLY_XLU_DISP++, self->flashEnvColor.r, self->flashEnvColor.g, self->flashEnvColor.b, 0);
            materialApplied = true;
        }
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 200, (s8)effect->primColor.a);
        Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(2.0f * effect->scale, 2.0f * effect->scale, 1.0f, MTXMODE_APPLY);
        LoadMatrix(&POLY_XLU_DISP, gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kFlashDL);
        FrameInterpolation_RecordCloseChild();
    }

    materialApplied = false;
    effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if (effect->type != EFFECT_LIGHT_RAYS) {
            continue;
        }
        FrameInterpolation_RecordOpenChild(effect, effect->type);
        if (!materialApplied) {
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetEnvColor(POLY_XLU_DISP++, (u8)effect->envColor.r, (u8)effect->envColor.g, (u8)effect->envColor.b, 0);
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kLightRayMaterialDL);
            materialApplied = true;
        }
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, (u8)effect->primColor.r, (u8)effect->primColor.g,
                        (u8)effect->primColor.b, (u8)effect->primColor.a);
        Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
        Matrix_RotateY(effect->rotationY, MTXMODE_APPLY);
        Matrix_RotateX(effect->rotationX, MTXMODE_APPLY);
        Matrix_RotateZ(effect->rotationZ, MTXMODE_APPLY);
        Matrix_Scale(effect->scale * 0.5f, effect->scale * 0.5f, effect->maxScale * effect->scale, MTXMODE_APPLY);
        Matrix_RotateX(kPi / 2, MTXMODE_APPLY);
        LoadMatrix(&POLY_XLU_DISP, gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)kLightRayDL);
        FrameInterpolation_RecordCloseChild();
    }

    effect = self->effect;
    for (s32 i = 0; i < OOTMM_CLEAR_TAG_EFFECT_COUNT; i++, effect++) {
        if (effect->type != EFFECT_SPLASH) {
            continue;
        }
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 200);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 200);
        gSPSegment(POLY_XLU_DISP++, 0x08, reinterpret_cast<uintptr_t>(sWaterSplashTextures[effect->actionTimer]));
        Gfx_SetupDL_61Xlu(gfxCtx);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BACK);

        for (s32 j = 0; j < 16; j++) {
            Vec3f offset;
            WaterBox* waterBox;
            FrameInterpolation_RecordOpenChild(effect, effect->type + (j << 4));
            Matrix_RotateY(2.0f * (j * kPi) * (1.0f / 16.0f), MTXMODE_NEW);
            MultVecZ(effect->maxScale, &offset);
            f32 ySurface = effect->pos.y;
            if (WaterBox_GetSurface1(play, &play->colCtx, effect->pos.x + offset.x, effect->pos.z + offset.z, &ySurface,
                                     &waterBox) &&
                ((effect->pos.y - ySurface) < 200.0f)) {
                Matrix_Translate(effect->pos.x + offset.x, ySurface, effect->pos.z + offset.z, MTXMODE_NEW);
                Matrix_RotateY(2.0f * (j * kPi) * (1.0f / 16.0f), MTXMODE_APPLY);
                Matrix_RotateX(effect->rotationX, MTXMODE_APPLY);
                Matrix_Scale(effect->scale, effect->scale, effect->scale, MTXMODE_APPLY);
                LoadMatrix(&POLY_XLU_DISP, gfxCtx);
                gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gEffWaterSplashDL);
            }
            FrameInterpolation_RecordCloseChild();
        }
    }

    CLOSE_DISPS(gfxCtx);
}

extern "C" Actor* OotmmClearTag_Spawn(PlayState* play, f32 x, f32 y, f32 z, s16 rotZ, s16 type) {
    if (gOotmmClearTagId < 0) {
        return nullptr;
    }
    return Actor_Spawn(&play->actorCtx, play, gOotmmClearTagId, x, y, z, 0, 0, rotZ, type);
}
