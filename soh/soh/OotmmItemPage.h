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
int OotmmItemPage_EquipOutlineIndex(uint8_t cButtonSlot);
void OotmmItemPage_UpdateCursor(struct PlayState* play);
void OotmmItemPage_Draw(struct PlayState* play);
void OotmmItemPage_DrawPageIndicator(struct PlayState* play);
// Prints the "page/pages (L)" label of a pause screen at a text cell.
void OotmmItemPage_DrawPageNumber(struct PlayState* play, int page, int pages, int column, int row);
const char* OotmmItemPage_HoveredName(void);
void OotmmItemPage_FillDpadDefaults(void);

#ifdef __cplusplus
}
#endif
