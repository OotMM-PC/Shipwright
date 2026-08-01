#pragma once

#include <stdint.h>

// PlayState is an anonymous typedef, so include this after z64.h.

#ifdef __cplusplus
extern "C" {
#endif

/// Whether the asset pack carries MM's world map, owl and map name textures.
int32_t OotmmSoaringMap_Available(void);
/// `owls` lists the selectable owl ids in MM's own order and `cursor` indexes that list.
void OotmmSoaringMap_Draw(PlayState* play, const uint8_t* owls, int32_t count, int32_t cursor);
void OotmmSoaringMap_CursorMoved(void);

#ifdef __cplusplus
}
#endif
