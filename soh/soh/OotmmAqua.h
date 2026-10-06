#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t gOotmmAquaId;

#ifdef Z64_H
typedef struct OotmmAqua {
    Actor actor;
    ColliderCylinder collider;
    void (*actionFunc)(struct OotmmAqua*, PlayState*);
    s16 counter;
    u8 alpha;
    s16 wobble;
} OotmmAqua;

void OotmmAqua_Init(Actor* thisx, PlayState* play);
void OotmmAqua_Destroy(Actor* thisx, PlayState* play);
void OotmmAqua_Update(Actor* thisx, PlayState* play);
void OotmmAqua_Draw(Actor* thisx, PlayState* play);
#endif

#ifdef __cplusplus
}
#endif
