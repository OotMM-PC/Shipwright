#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Actor;
struct PlayState;

int32_t OotmmSongEvents_Enabled(void);
int32_t OotmmSongEvents_Action(struct Actor* actor, struct PlayState* play, int32_t vanillaAction);
int32_t OotmmSongEvents_Song(struct Actor* actor, struct PlayState* play);
int32_t OotmmSongEvents_HandleCheckPlayed(struct PlayState* play);
int32_t OotmmSongEvents_GuideNotes(struct PlayState* play, uint8_t* notes, int32_t max);

#ifdef __cplusplus
}
#endif
