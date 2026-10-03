#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "z64.h"

void OotmmCustomWarp_Init(Actor* thisx, PlayState* play);
void OotmmCustomWarp_Update(Actor* thisx, PlayState* play);
void OotmmCustomWarp_Draw(Actor* thisx, PlayState* play);
void OotmmRegionState_SetDungeonFlags(int dungeonId, int flags);
void OotmmRegionState_ApplyBossWarp(int bossId);
void OotmmRegionState_ApplyBossClear(int dungeonId);

extern s16 gOotmmCustomWarpId;

#ifdef __cplusplus
}

void OotmmRegionState_Init();
void OotmmRegionState_InitializeNewSave();
#endif
