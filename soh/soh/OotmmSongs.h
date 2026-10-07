#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OotmmSongId {
    OOTMM_SONG_NONE,
    OOTMM_SONG_SOARING,
    OOTMM_SONG_ELEGY,
    OOTMM_SONG_DOUBLE_TIME,
    OOTMM_SONG_HEALING,
    OOTMM_SONG_AWAKENING,
    OOTMM_SONG_GORON,
    OOTMM_SONG_GORON_HALF,
    OOTMM_SONG_ZORA,
    OOTMM_SONG_ORDER,
    OOTMM_SONG_MAX,
} OotmmSongId;

#define OOTMM_SONG_COUNT (OOTMM_SONG_MAX - 1)

#define OOTMM_SONG_STAFF_BASE 0x80

uint16_t OotmmSongs_AvailableMask(void);
uint16_t OotmmSongs_OwnedMask(void);
uint16_t OotmmSongs_ScarecrowBlockMask(void);
int32_t OotmmSongs_FanfareSeqId(int32_t song);
const char* OotmmSongs_Name(int32_t song);
int32_t OotmmSongs_MenuIsOpen(void);

void Ootmm_OcarinaSetSongPlayback(int32_t song);

#ifdef __cplusplus
}
#endif
