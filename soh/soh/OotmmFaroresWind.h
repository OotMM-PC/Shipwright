#pragma once

#include <stdint.h>

#define OOTMM_ENTR_FW_CROSS 0xFFFD

#ifdef __cplusplus
#include <string_view>

bool OotmmFaroresWind_IsSharedKey(std::string_view key);
uint32_t OotmmFaroresWind_ReadShared(std::string_view key);
void OotmmFaroresWind_WriteShared(std::string_view key, uint32_t owner);
bool OotmmFaroresWind_ApplyArrival();

extern "C" {
#endif

int32_t OotmmFaroresWind_PerAge(void);
void OotmmFaroresWind_SwapAge(void);
int32_t OotmmFaroresWind_IsCrossEntrance(int32_t entranceIndex);
int32_t OotmmFaroresWind_BeginCrossWarp(void);

#ifdef __cplusplus
}
#endif
