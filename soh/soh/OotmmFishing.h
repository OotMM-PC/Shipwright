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

#ifdef __cplusplus
}
#endif
