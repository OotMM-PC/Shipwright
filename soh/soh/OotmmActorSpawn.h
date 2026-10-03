#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

int32_t OotmmActorSpawn_Rewrite(struct PlayState* play, int16_t* actorId, int16_t* params);

#ifdef __cplusplus
}
#endif
