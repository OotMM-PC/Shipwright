#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Actor;
struct PlayState;

int32_t OotmmSongEvents_Enabled(void);
/// The ocarina action for this spot: the shuffled song's check action.
int32_t OotmmSongEvents_Action(struct Actor* actor, struct PlayState* play, int32_t vanillaAction);
/// The assigned song id (0-19) for the tag glyph, or -1 when unshuffled.
int32_t OotmmSongEvents_Song(struct Actor* actor, struct PlayState* play);
/// Completes a staff whose asked MM song was just played; returns 0 otherwise so the
/// song keeps its free-play behavior.
int32_t OotmmSongEvents_HandleCheckPlayed(struct PlayState* play);
/// Fills the staff's asked song when the player can play it; returns the note count, else 0.
int32_t OotmmSongEvents_GuideNotes(struct PlayState* play, uint8_t* notes, int32_t max);

#ifdef __cplusplus
}
#endif
