#pragma once

#include "OotmmCustomItems.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

typedef enum OotmmScaleTierId {
    OOTMM_SCALE_NONE,
    OOTMM_SCALE_BRONZE,
    OOTMM_SCALE_SILVER,
    OOTMM_SCALE_GOLDEN,
} OotmmScaleTierId;

// The Bronze Scale has no inventory slot; this id exists only so the equipment page can name it.
#define ITEM_OOTMM_SCALE_BRONZE ITEM_OOTMM_MAX

void OotmmScales_Init(void);

int OotmmScales_Tier(void);
/// True when the scale slot is filled, counting the swim-only Bronze Scale that leaves UPG_SCALE at zero.
int OotmmScales_SlotFilled(void);
/// Draws the bronze scale icon in the equipment grid; 1 when it took over the slot.
int OotmmScales_DrawSlotIcon(struct PlayState* play);
/// Draws "Bronze Scale" into the pause info panel; 1 when it replaced the vanilla name texture.
int OotmmScales_DrawSlotName(struct PlayState* play);

#ifdef __cplusplus
}
#endif
