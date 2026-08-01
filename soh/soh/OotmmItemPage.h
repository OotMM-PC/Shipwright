#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlayState;

int OotmmItemPage_Active(void);
int OotmmItemPage_PageCount(void);
int OotmmItemPage_CurrentPage(void);
int OotmmItemPage_ConsumePageToggle(void);
/// Inventory grid index to source the equipped-item outline from, or -1 to hide it.
int OotmmItemPage_EquipOutlineIndex(uint8_t cButtonSlot);
void OotmmItemPage_UpdateCursor(struct PlayState* play);
void OotmmItemPage_Draw(struct PlayState* play);
void OotmmItemPage_DrawPageIndicator(struct PlayState* play);
const char* OotmmItemPage_HoveredName(void);

#ifdef __cplusplus
}
#endif
