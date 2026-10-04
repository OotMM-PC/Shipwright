#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t gOotmmIceFloeId;

#ifdef Z64_H
typedef struct OotmmIceFloe {
    DynaPolyActor dyna;
    void (*actionFunc)(struct OotmmIceFloe*, PlayState*);
    int32_t timer;
} OotmmIceFloe;

void OotmmIceFloe_Init(Actor* thisx, PlayState* play);
void OotmmIceFloe_Destroy(Actor* thisx, PlayState* play);
void OotmmIceFloe_Update(Actor* thisx, PlayState* play);
void OotmmIceFloe_Draw(Actor* thisx, PlayState* play);
void OotmmIceFloe_CheckArrow(Actor* arrow, PlayState* play);
#endif

#ifdef __cplusplus
}
#endif
