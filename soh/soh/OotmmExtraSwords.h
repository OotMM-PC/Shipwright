#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t OotmmExtraSwords_Level(void);
uint8_t OotmmExtraSwords_SwordDamage(int32_t dmgFlags, uint8_t damage);
float OotmmExtraSwords_KokiriLength(float length);
int32_t OotmmExtraSwords_KokiriSpinDamage(int32_t great);
const char* OotmmExtraSwords_Icon(uint8_t item);
#ifdef Z64_H
Gfx* OotmmExtraSwords_Hand(PlayState* play, Gfx* closedHand);
Gfx* OotmmExtraSwords_Sheath(PlayState* play, int32_t shield, int32_t shieldOnBack, int32_t swordInHand);
#endif
void OotmmExtraSwords_Init(void);

#ifdef __cplusplus
}
#endif
