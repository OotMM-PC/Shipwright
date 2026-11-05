#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OotmmCustomItemId {
    ITEM_OOTMM_FIRST = 0x9E,
    ITEM_OOTMM_MASK_BLAST = ITEM_OOTMM_FIRST,
    ITEM_OOTMM_MASK_STONE,
    ITEM_OOTMM_MASK_KAMARO,
    ITEM_OOTMM_POWDER_KEG,
    ITEM_OOTMM_GREAT_FAIRY_SWORD,
    ITEM_OOTMM_MAX,
} OotmmCustomItemId;

#define OOTMM_CUSTOM_ITEM_COUNT (ITEM_OOTMM_MAX - ITEM_OOTMM_FIRST)

#define SLOT_OOTMM_FIRST 0x80
#define SLOT_OOTMM_MAX (SLOT_OOTMM_FIRST + OOTMM_CUSTOM_ITEM_COUNT)

void OotmmCustomItems_Init(void);

int OotmmCustomItems_IsCustomItem(uint8_t item);
int OotmmCustomItems_IsCustomSlot(uint8_t slot);
uint8_t OotmmCustomItems_SlotOf(uint8_t item);
uint8_t OotmmCustomItems_ItemInSlot(uint8_t slot);

const char* OotmmCustomItems_IconPath(uint8_t item);
void* OotmmCustomItems_ButtonIcon(uint8_t item);
const char* OotmmCustomItems_DisplayName(uint8_t item);

uint8_t OotmmCustomItems_IdForItemId(const char* ootmmItemId);

int OotmmCustomItems_Owned(uint8_t item);
int OotmmCustomItems_KegAmmo(void);
void OotmmCustomItems_SetKegAmmo(int ammo);
int OotmmCustomItems_CanCarryKeg(void);
void OotmmCustomItems_UpdateKegStock(void);
int OotmmCustomItems_MedigoronKegConversation(void);
int OotmmCustomItems_MedigoronSellsKeg(void);
void OotmmCustomItems_ResetMedigoronIntro(void);
int OotmmCustomItems_MedigoronKegPrice(void);
void OotmmCustomItems_BuyKegFromMedigoron(void);
int OotmmCustomItems_AmmoOf(uint8_t item);
int OotmmCustomItems_UsableNow(uint8_t item);
uint8_t OotmmCustomItems_ButtonIconShade(uint8_t item);
uint32_t OotmmCustomItems_OwnedCount(uint8_t item);
int OotmmCustomItems_OwnedItemCount(void);
uint8_t OotmmCustomItems_OwnedItemAt(int index);

int32_t OotmmCustomItems_ItemAction(uint8_t item);
const char* OotmmCustomItems_MaskDList(uint8_t mask);
int OotmmCustomItems_Usable(uint8_t item);

#ifdef __cplusplus
}
#endif
