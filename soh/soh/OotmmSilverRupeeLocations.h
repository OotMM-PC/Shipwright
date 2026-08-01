#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Actor;
struct PlayState;

/// Claims the id OoTMM gives the silver rupee being spawned, or -1 when it has none.
int32_t OotmmSilverRupeeLocations_Claim(struct PlayState* play, struct Actor* actor);
int32_t OotmmSilverRupeeLocations_Taken(struct PlayState* play, int32_t id);
void OotmmSilverRupeeLocations_MarkTaken(struct PlayState* play, int32_t id);
int32_t OotmmSilverRupeeLocations_TakenCount(struct PlayState* play, int32_t room, int32_t total);

#ifdef __cplusplus
}
#endif
