#pragma once

#include <stdint.h>

#define OOTMM_ENTR_DUNGEON_RESPAWN 0xFFFC

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

int32_t OotmmDungeonRespawn_Available(void);
int32_t OotmmDungeonRespawn_Warp(struct PlayState* play);
int32_t OotmmDungeonRespawn_RedirectRespawn(struct PlayState* play);
int32_t OotmmDungeonRespawn_ApplyArrival(void);
void OotmmDungeonRespawn_Clear(void);
void OotmmDungeonRespawn_SaveForFaroresWind(void);
void OotmmDungeonRespawn_RestoreForFaroresWind(void);

#ifdef __cplusplus
}

void OotmmDungeonRespawn_Init();
#endif
