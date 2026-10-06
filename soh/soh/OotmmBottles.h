#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OotmmBottleItemId {
    ITEM_OOTMM_BOTTLE_FIRST = 0xA4,
    ITEM_OOTMM_MAGIC_MUSHROOM = ITEM_OOTMM_BOTTLE_FIRST,
    ITEM_OOTMM_CHATEAU,
    ITEM_OOTMM_GOLD_DUST,
    ITEM_OOTMM_SEAHORSE,
    ITEM_OOTMM_DEKU_PRINCESS,
    ITEM_OOTMM_SPRING_WATER,
    ITEM_OOTMM_SPRING_WATER_HOT,
    ITEM_OOTMM_ZORA_EGG,
    ITEM_OOTMM_BOTTLE_MAX,
} OotmmBottleItemId;

#define EXCH_ITEM_OOTMM_BOTTLE_FIRST 0x1E
#define EXCH_ITEM_OOTMM_MAGIC_MUSHROOM (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 0)
#define EXCH_ITEM_OOTMM_CHATEAU (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 1)
#define EXCH_ITEM_OOTMM_GOLD_DUST (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 2)
#define EXCH_ITEM_OOTMM_SEAHORSE (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 3)
#define EXCH_ITEM_OOTMM_DEKU_PRINCESS (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 4)
#define EXCH_ITEM_OOTMM_SPRING_WATER (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 5)
#define EXCH_ITEM_OOTMM_SPRING_WATER_HOT (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 6)
#define EXCH_ITEM_OOTMM_ZORA_EGG (EXCH_ITEM_OOTMM_BOTTLE_FIRST + 7)

struct PlayState;
struct Player;

void OotmmBottles_Init(void);
int32_t OotmmBottles_Shared(void);

int32_t OotmmBottles_IsItem(uint8_t item);
int32_t OotmmBottles_IsTradeBottleSlot(int32_t slot);
int32_t OotmmBottles_IsAction(int32_t itemAction);
int32_t OotmmBottles_ItemAction(uint8_t item);
int32_t OotmmBottles_IsExchange(int32_t exchangeItemId);
const char* OotmmBottles_IconPath(uint8_t item);
const uint8_t* OotmmBottles_LiquidColor(int32_t itemAction);

int32_t OotmmBottles_ExchangeIndex(int32_t itemAction);
int32_t OotmmBottles_AlwaysExchange(int32_t itemAction);
int32_t OotmmBottles_TalkAccepts(int32_t wanted, int32_t offered);
int32_t OotmmBottles_UseType(int32_t itemAction);
int32_t OotmmBottles_Drink(int32_t itemAction);
void OotmmBottles_Pour(struct PlayState* play, struct Player* player, float x, float y, float z);

int32_t OotmmBottles_ChateauActive(void);

uint32_t OotmmBottles_ContentOf(uint8_t item);
uint8_t OotmmBottles_ItemFor(uint32_t content);
uint32_t OotmmBottles_ReadSlot(int32_t bottle);
void OotmmBottles_WriteSlot(int32_t bottle, uint32_t content);
int32_t OotmmBottles_GiveNew(uint8_t item);
int32_t OotmmBottles_FillEmpty(uint8_t item);
int32_t OotmmBottles_Holds(uint8_t item);
int32_t OotmmBottles_ConsumeFairy(struct PlayState* play);
void OotmmBottles_GiveRutoLetter(struct PlayState* play);
uint8_t OotmmBottles_TradeSlotBottle(uint8_t slot);
int32_t OotmmBottles_HasSpecific(uint8_t item);
int32_t OotmmBottles_BeggarHasSellable(void);
int32_t OotmmBottles_BeggarRefuses(int32_t exchangeItemId);
int32_t OotmmBottles_BeggarOffer(int32_t exchangeItemId);
int32_t OotmmBottles_BeggarPayout(int16_t vanillaReward);

#ifdef __cplusplus
}
#endif
