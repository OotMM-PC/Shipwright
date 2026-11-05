#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <libultraship/bridge/OotmmGameState.h>

#include <string>

extern "C" {
#endif

int32_t OotmmSession_IsActive(void);
int32_t OotmmSession_ApplyFreestandingCosmetics(void);
int32_t OotmmSession_RequestFileSelection(int32_t slot);
int32_t OotmmSession_FileSelectionPending(void);
int32_t OotmmSession_RequestFileCopy(int32_t source, int32_t target);
int32_t OotmmSession_TryBootDirectly(void* gameState);
void OotmmSession_InitializeNewSave(void);
void OotmmSession_FixLoadSpawn(void);
int32_t OotmmSession_TryStartLoadSpawnHandoff(void);
void OotmmSession_NotePlayerExitTransition(void);
void OotmmSession_PrepareGrottoReturn(void);
int32_t OotmmSession_WallmasterTakePlayer(struct PlayState* play, struct Actor* wallmaster);
void OotmmSession_ApplyDeathRespawn(void);
int32_t OotmmSession_ReturnToSpawn(void);
int32_t OotmmSession_ApplyResolvedOotEntrance(uint32_t entrance);
int32_t OotmmSession_RedirectBossWarp(uint32_t bossId);
int32_t OotmmSession_PrepareSongWarp(uint32_t songId);
int32_t OotmmSession_IsSceneMasterQuest(int32_t sceneNum);
int32_t OotmmSession_BoolSetting(const char* key);

#ifdef __cplusplus
}

bool OotmmSession_BeginCrossGameTransition(const Ship::OotmmEntranceMapping& mapping);
bool OotmmSession_IsCrossGameTransitionPending();
void OotmmSession_Init();
const Ship::OotmmGameState& OotmmSession_GetState();
std::string OotmmSession_GetSaveSubdirectory();
bool OotmmSession_HasEntranceShuffle();
#endif
