#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int16_t gOotmmElegyShellId;

#ifdef Z64_H
typedef struct OotmmElegyShell {
    Actor actor;
    ColliderCylinder collider;
    u8 state;
    u8 framesUntilNextState;
    s16 alpha;
} OotmmElegyShell;

void OotmmElegyShell_Init(Actor* thisx, PlayState* play);
void OotmmElegyShell_Destroy(Actor* thisx, PlayState* play);
void OotmmElegyShell_Update(Actor* thisx, PlayState* play);
void OotmmElegyShell_Draw(Actor* thisx, PlayState* play);
void OotmmElegyShell_Place(PlayState* play);
#endif

#ifdef __cplusplus
}
#endif
