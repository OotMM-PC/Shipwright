#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Free item ids above ITEM_ROCS_FEATHER (0x9D).
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

// Above the 24 vanilla inventory slots, below SLOT_NONE (0xFF).
#define SLOT_OOTMM_FIRST 0x80
#define SLOT_OOTMM_MAX (SLOT_OOTMM_FIRST + OOTMM_CUSTOM_ITEM_COUNT)

void OotmmCustomItems_Init(void);

int OotmmCustomItems_IsCustomItem(uint8_t item);
int OotmmCustomItems_IsCustomSlot(uint8_t slot);
uint8_t OotmmCustomItems_SlotOf(uint8_t item);
uint8_t OotmmCustomItems_ItemInSlot(uint8_t slot);

const char* OotmmCustomItems_IconPath(uint8_t item);
/// Icon for a button slot: our texture for custom ids, gItemIcons otherwise.
void* OotmmCustomItems_ButtonIcon(uint8_t item);
const char* OotmmCustomItems_DisplayName(uint8_t item);

/// Item id for an OoTMM inventory id, or ITEM_NONE when it has no custom item.
uint8_t OotmmCustomItems_IdForItemId(const char* ootmmItemId);

int OotmmCustomItems_Owned(uint8_t item);
/// Kegs in hand; OoTMM caps this at one.
int OotmmCustomItems_KegAmmo(void);
void OotmmCustomItems_SetKegAmmo(int ammo);
int OotmmCustomItems_CanCarryKeg(void);
/// Grants the first keg once the cross-game item is owned.
void OotmmCustomItems_UpdateKegStock(void);
int OotmmCustomItems_MedigoronSellsKeg(void);
int OotmmCustomItems_MedigoronKegPrice(void);
void OotmmCustomItems_BuyKegFromMedigoron(void);
/// Rounds carried, or -1 for items that have no ammo.
int OotmmCustomItems_AmmoOf(uint8_t item);
int OotmmCustomItems_UsableNow(uint8_t item);
uint8_t OotmmCustomItems_ButtonIconShade(uint8_t item);
uint32_t OotmmCustomItems_OwnedCount(uint8_t item);
int OotmmCustomItems_OwnedItemCount(void);
uint8_t OotmmCustomItems_OwnedItemAt(int index);

/// PlayerItemAction for a custom item, or -1 when the id is not one of ours.
int32_t OotmmCustomItems_ItemAction(uint8_t item);
const char* OotmmCustomItems_MaskDList(uint8_t mask);
int OotmmCustomItems_Usable(uint8_t item);

#ifdef __cplusplus
}
#endif
