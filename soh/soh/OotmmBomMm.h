#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t gOotmmBomMmId;

enum {
    OOTMM_BOM_MM_TYPE_BOMB = 0,
    OOTMM_BOM_MM_TYPE_POWDER_KEG = 1,
    OOTMM_BOM_MM_TYPE_BLAST = 2
};

#ifdef Z64_H
struct OotmmBomMm;

typedef void (*OotmmBomMmActionFunc)(struct OotmmBomMm*, PlayState*);

typedef struct OotmmBomMm {
    Actor actor;
    ColliderCylinder collider1;
    ColliderJntSph collider2;
    ColliderJntSphElement collider2Elements[1];
    s16 timer;
    s16 flashSpeedScale;
    f32 flashIntensity;
    u8 bumpOn;
    u8 isPowderKeg;
    s16 rollAngle;
    u8 sinkTimer;
    OotmmBomMmActionFunc actionFunc;
} OotmmBomMm;

void OotmmBomMm_Init(Actor* thisx, PlayState* play);
void OotmmBomMm_Destroy(Actor* thisx, PlayState* play);
void OotmmBomMm_Update(Actor* thisx, PlayState* play);
void OotmmBomMm_Draw(Actor* thisx, PlayState* play);
int OotmmBomMm_IsPowderKeg(Actor* actor);
int OotmmBomMm_IsCustomBomb(Actor* actor);
#endif

#ifdef __cplusplus
}
#endif
