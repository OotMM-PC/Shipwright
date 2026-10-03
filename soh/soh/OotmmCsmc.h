#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <libultraship/bridge/OotmmCsmc.h>

extern "C" {
#endif

struct EnBox;
struct PlayState;

/// Owns the look of shuffled chests: returns nonzero when it applied size and
/// display lists, replacing the vanilla appearance path.
int32_t OotmmCsmc_UpdateChest(struct EnBox* chest, struct PlayState* play);

#ifdef __cplusplus
}

/// Renders a chest as the given class regardless of placements (test chests).
void OotmmCsmc_SetChestOverride(EnBox* chest, Ship::OotmmCsmcClass itemClass);
void OotmmCsmc_ClearChestOverrides();
#endif
