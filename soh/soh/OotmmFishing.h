#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// A physically caught fish keeps priority; no-op unless pond fish shuffle is active.
void OotmmFishing_PeekOnHandFish(float* onHandLength, uint8_t* onHandIsLoach);

void OotmmFishing_TakeOnHandFish(float* onHandLength, uint8_t* onHandIsLoach);

void OotmmFishing_DiscardOnHandFish(float* onHandLength, uint8_t* onHandIsLoach);

void OotmmFishing_ReleaseOnHandFish(void);

struct Actor;
struct PlayState;

int OotmmFishing_FishSpecies(struct Actor* fish);
void OotmmFishing_InitFishLength(struct Actor* fish, float* length);
int OotmmFishing_DrawFish(struct Actor* fish, struct PlayState* play);
int OotmmFishing_CatchFish(struct Actor* fish, struct PlayState* play);
int OotmmFishing_SpawnLoaches(int vanilla);

#ifdef __cplusplus
}
#endif
