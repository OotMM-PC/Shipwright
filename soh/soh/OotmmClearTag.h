#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t gOotmmClearTagId;

enum {
    OOTMM_CLEAR_TAG_SMALL_EXPLOSION = 0,
    OOTMM_CLEAR_TAG_LARGE_EXPLOSION = 1,
    OOTMM_CLEAR_TAG_POP = 2,
    OOTMM_CLEAR_TAG_SMALL_LIGHT_RAYS = 3,
    OOTMM_CLEAR_TAG_LARGE_LIGHT_RAYS = 4,
    OOTMM_CLEAR_TAG_SPLASH = 35,
    OOTMM_CLEAR_TAG_SMOKE = 200
};

#ifdef Z64_H
#define OOTMM_CLEAR_TAG_EFFECT_COUNT 103

typedef struct OotmmClearTagColor {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} OotmmClearTagColor;

typedef struct OotmmClearTagEffect {
    u8 type;
    u8 actionTimer;
    Vec3f pos;
    Vec3f velocity;
    Vec3f accel;
    OotmmClearTagColor primColor;
    OotmmClearTagColor envColor;
    s16 bounces;
    s16 effectsTimer;
    s16 lightRayAlpha;
    s16 lightRayAlphaDecrementSpeed;
    f32 scale;
    f32 maxScale;
    f32 rotationY;
    f32 rotationX;
    f32 rotationZ;
    f32 maxScaleTarget;
    f32 smokeScaleY;
    f32 smokeScaleX;
} OotmmClearTagEffect;

typedef struct OotmmClearTag {
    Actor actor;
    OotmmClearTagEffect effect[OOTMM_CLEAR_TAG_EFFECT_COUNT];
    s16 activeTimer;
    Color_RGBA8 flashEnvColor;
} OotmmClearTag;

void OotmmClearTag_Init(Actor* thisx, PlayState* play);
void OotmmClearTag_Destroy(Actor* thisx, PlayState* play);
void OotmmClearTag_Update(Actor* thisx, PlayState* play);
void OotmmClearTag_Draw(Actor* thisx, PlayState* play);
Actor* OotmmClearTag_Spawn(PlayState* play, f32 x, f32 y, f32 z, s16 rotZ, s16 type);
#endif

#ifdef __cplusplus
}
#endif
