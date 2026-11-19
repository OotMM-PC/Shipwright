#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

void OotmmSongs_NotePlayed(int32_t song);
const OcarinaSongInfo* AudioOcarina_OotmmSongNotes(int32_t song);
int32_t OotmmSongs_Played(void);
void OotmmSongs_ClearPlayed(void);

void OotmmSongs_LoadPlayedText(void);
int32_t OotmmSongs_EffectActorId(void);
uint16_t OotmmSongs_EffectActorParams(void);

void OotmmSongs_Begin(PlayState* play);
int32_t OotmmSongs_OfferAgeChange(PlayState* play);
void OotmmSongs_UpdateLastSafePos(PlayState* play, Player* player);
void OotmmSongs_Update(PlayState* play, Player* player);
void OotmmSongs_DrawMenu(PlayState* play);

#ifdef __cplusplus
}
#endif
