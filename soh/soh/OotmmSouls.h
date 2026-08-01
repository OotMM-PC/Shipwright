#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

/// Returns 1 when the seed withholds this actor's Soul, in which case the spawn must be abandoned.
int32_t OotmmSouls_SuppressSpawn(struct PlayState* play, int16_t actorId, int16_t params);
/// Forgets which enemies were withheld; call before a room's actor list is spawned.
void OotmmSouls_ResetRoomState(void);
int32_t OotmmSouls_RoomClearBlocked(void);
/// Returns 1 when the seed withholds the named OoT Soul, written without its "OOT_SOUL_" prefix.
int32_t OotmmSouls_Withheld(const char* soul);

#ifdef __cplusplus
}
#endif
