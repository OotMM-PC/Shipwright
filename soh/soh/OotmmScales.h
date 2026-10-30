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

#define ITEM_OOTMM_SCALE_BRONZE ITEM_OOTMM_MAX

void OotmmScales_Init(void);

int OotmmScales_Tier(void);
int OotmmScales_SlotFilled(void);
int OotmmScales_DrawSlotIcon(struct PlayState* play);
int OotmmScales_DrawSlotName(struct PlayState* play);

#ifdef __cplusplus
}
#endif
