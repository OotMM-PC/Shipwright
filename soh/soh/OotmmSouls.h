#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

int32_t OotmmSouls_SuppressSpawn(struct PlayState* play, int16_t actorId, int16_t params);
void OotmmSouls_ResetRoomState(void);
int32_t OotmmSouls_RoomClearBlocked(void);
int32_t OotmmSouls_Withheld(const char* soul);

#ifdef __cplusplus
}
#endif
