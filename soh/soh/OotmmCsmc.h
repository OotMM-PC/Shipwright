#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EnBox;
struct PlayState;

/// Owns the look of shuffled chests: returns nonzero when it applied size and
/// display lists, replacing the vanilla appearance path.
int32_t OotmmCsmc_UpdateChest(struct EnBox* chest, struct PlayState* play);

#ifdef __cplusplus
}
#endif
