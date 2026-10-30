#pragma once

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

int32_t OotmmSoaringMap_Available(void);
void OotmmSoaringMap_Draw(PlayState* play, const uint8_t* owls, int32_t count, int32_t cursor);
void OotmmSoaringMap_CursorMoved(void);

#ifdef __cplusplus
}
#endif
