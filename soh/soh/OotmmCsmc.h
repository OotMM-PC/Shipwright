#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <libultraship/bridge/OotmmCsmc.h>

extern "C" {
#endif

struct Actor;
struct EnBox;
struct PlayState;

int32_t OotmmCsmc_UpdateChest(struct EnBox* chest, struct PlayState* play);

#ifdef __cplusplus
}

void OotmmCsmc_SetChestOverride(EnBox* chest, Ship::OotmmCsmcClass itemClass);
void OotmmCsmc_ClearChestOverrides();

void OotmmCsmc_DrawGlitter(PlayState* play, Actor* actor, const Ship::OotmmPlacement& placement);

void* OotmmCsmc_ContainerDL(const Ship::OotmmPlacement& placement, const char* family);
void* OotmmCsmc_SkulltulaDL(const Ship::OotmmPlacement* placement, const char* part);
void* OotmmCsmc_CowRingDL(const Ship::OotmmPlacement* placement, bool collected, void* vanilla);
#endif
