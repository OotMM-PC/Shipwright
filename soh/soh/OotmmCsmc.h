#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <libultraship/bridge/OotmmCsmc.h>

extern "C" {
#endif

struct Actor;
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

/// OoTMM's glitter marker over a hidden check, tinted by the class of its item.
void OotmmCsmc_DrawGlitter(PlayState* play, Actor* actor, const Ship::OotmmPlacement& placement);

void* OotmmCsmc_ContainerDL(const Ship::OotmmPlacement& placement, const char* family);
void* OotmmCsmc_SkulltulaDL(const Ship::OotmmPlacement* placement, const char* part);
void* OotmmCsmc_CowRingDL(const Ship::OotmmPlacement* placement, bool collected, void* vanilla);
#endif
