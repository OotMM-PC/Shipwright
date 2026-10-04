#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t OotmmAgeless_Strength(void);
int32_t OotmmAgeless_EquipmentVisible(void);
int32_t OotmmAgeless_ChildTrade(void);
int16_t OotmmAgeless_FixDrop(int16_t dropId);

#ifdef __cplusplus
}

void OotmmAgeless_Init();
#endif
