#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Scene backing an OoTMM dungeon code such as "GTG", or -1 when the code names no OoT dungeon.
int OotmmDungeons_SceneForCode(const char* code);
int OotmmDungeons_MaxSmallKeys(int sceneNum);
int OotmmDungeons_SmallKeysRemoved(int sceneNum);
int OotmmDungeons_DoorIsForcedOpen(int sceneNum, int switchFlag);
int OotmmDungeons_BossDoorIsOpen(int sceneNum);

#ifdef __cplusplus
}
#endif
