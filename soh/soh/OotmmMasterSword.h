#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Actor;
struct PlayState;

int32_t OotmmMasterSword_CanTimeTravel(void);
int32_t OotmmMasterSword_PedestalReady(struct Actor* actor, struct PlayState* play);
int32_t OotmmMasterSword_SwapAgeEquipment(void);

#ifdef __cplusplus
}

void OotmmMasterSword_InitializeNewSave();
void OotmmMasterSword_Init();
#endif
