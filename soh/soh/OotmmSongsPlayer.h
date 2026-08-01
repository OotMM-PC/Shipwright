#pragma once

#include <stdint.h>

// PlayState and Player are anonymous typedefs, so include this after z64.h.

#ifdef __cplusplus
extern "C" {
#endif

/// Records the MM song the ocarina just matched.
void OotmmSongs_NotePlayed(int32_t song);
int32_t OotmmSongs_Played(void);
void OotmmSongs_ClearPlayed(void);

/// Replaces the message buffer with "You played the <song>." for the song just played.
void OotmmSongs_LoadPlayedText(void);
/// Ocarina effect actor for the song just played, or -1 when it has none.
int32_t OotmmSongs_EffectActorId(void);
uint16_t OotmmSongs_EffectActorParams(void);

/// Starts the song's effect once its textbox is done; owns msgCtx.ocarinaMode from here on.
void OotmmSongs_Begin(PlayState* play);
void OotmmSongs_Update(PlayState* play, Player* player);
void OotmmSongs_DrawMenu(PlayState* play);

#ifdef __cplusplus
}
#endif
