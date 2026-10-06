#include "OotmmBottles.h"

#include "OotmmAqua.h"
#include "OotmmItemApply.h"
#include "OotmmSession.h"

#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include <libultraship/bridge/OotmmBottles.h>

#include <array>
#include <cstring>
#include <string>
#include <utility>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"

extern PlayState* gPlayState;
}

namespace {

using Content = Ship::OotmmBottleContent;

struct BottleItem {
    uint8_t Item;
    const char* Icon;
    const char* Name;
    uint8_t Color[3];
};

constexpr std::array<BottleItem, ITEM_OOTMM_BOTTLE_MAX - ITEM_OOTMM_BOTTLE_FIRST> kItems = { {
    { ITEM_OOTMM_MAGIC_MUSHROOM,
      "__OTR__mm_icon_item_static_yar/gItemIconBottledMushroomTex",
      "__OTR__mm_item_name_static/gItemNameMagicalMushroomENGTex",
      { 131, 0, 174 } },
    { ITEM_OOTMM_CHATEAU,
      "__OTR__mm_icon_item_static_yar/gItemIconChateauRomaniTex",
      "__OTR__mm_item_name_static/gItemNameChateauRomaniENGTex",
      { 255, 255, 255 } },
    { ITEM_OOTMM_GOLD_DUST,
      "__OTR__mm_icon_item_static_yar/gItemIconBottledGoldDustTex",
      "__OTR__mm_item_name_static/gItemNameGoldDustENGTex",
      { 255, 180, 0 } },
    { ITEM_OOTMM_SEAHORSE,
      "__OTR__mm_icon_item_static_yar/gItemIconBottledSeahorseTex",
      "__OTR__mm_item_name_static/gItemNameSeaHorseENGTex",
      { 252, 238, 0 } },
    { ITEM_OOTMM_DEKU_PRINCESS,
      "__OTR__mm_icon_item_static_yar/gItemIconBottledDekuPrincessTex",
      "__OTR__mm_item_name_static/gItemNameDekuPrincessENGTex",
      { 248, 200, 0 } },
    { ITEM_OOTMM_SPRING_WATER,
      "__OTR__mm_icon_item_static_yar/gItemIconSpringWaterTex",
      "__OTR__mm_item_name_static/gItemNameSpringWaterENGTex",
      { 136, 192, 255 } },
    { ITEM_OOTMM_SPRING_WATER_HOT,
      "__OTR__mm_icon_item_static_yar/gItemIconHotSpringWaterTex",
      "__OTR__mm_item_name_static/gItemNameHotSpringWaterENGTex",
      { 136, 192, 255 } },
    { ITEM_OOTMM_ZORA_EGG,
      "__OTR__mm_icon_item_static_yar/gItemIconBottledZoraEggTex",
      "__OTR__mm_item_name_static/gItemNameZoraEggENGTex",
      { 184, 232, 232 } },
} };

constexpr std::array<std::pair<uint8_t, Content>, 21> kContents = { {
    { ITEM_BOTTLE, Content::Empty },
    { ITEM_POTION_RED, Content::PotionRed },
    { ITEM_POTION_GREEN, Content::PotionGreen },
    { ITEM_POTION_BLUE, Content::PotionBlue },
    { ITEM_FAIRY, Content::Fairy },
    { ITEM_FISH, Content::Fish },
    { ITEM_MILK_BOTTLE, Content::Milk },
    { ITEM_MILK_HALF, Content::MilkHalf },
    { ITEM_LETTER_RUTO, Content::RutoLetter },
    { ITEM_BLUE_FIRE, Content::BlueFire },
    { ITEM_BUG, Content::Bugs },
    { ITEM_POE, Content::Poe },
    { ITEM_BIG_POE, Content::BigPoe },
    { ITEM_OOTMM_MAGIC_MUSHROOM, Content::Mushroom },
    { ITEM_OOTMM_CHATEAU, Content::Chateau },
    { ITEM_OOTMM_GOLD_DUST, Content::GoldDust },
    { ITEM_OOTMM_SEAHORSE, Content::Seahorse },
    { ITEM_OOTMM_DEKU_PRINCESS, Content::DekuPrincess },
    { ITEM_OOTMM_SPRING_WATER, Content::SpringWater },
    { ITEM_OOTMM_SPRING_WATER_HOT, Content::SpringWaterHot },
    { ITEM_OOTMM_ZORA_EGG, Content::ZoraEgg },
} };

struct BeggarSale {
    int32_t Exchange;
    uint8_t Item;
    const char* Line;
    int16_t Price;
};

constexpr std::array<BeggarSale, 8> kBeggarSales = { {
    { EXCH_ITEM_FISH, ITEM_FISH, "a %pFish%w!", 100 },
    { EXCH_ITEM_BLUE_FIRE, ITEM_BLUE_FIRE, "that's %cBlue Fire%w!", 150 },
    { EXCH_ITEM_BUG, ITEM_BUG, "it's a cute little %gBug%w!", 50 },
    { EXCH_ITEM_OOTMM_MAGIC_MUSHROOM, ITEM_OOTMM_MAGIC_MUSHROOM, "an exotic %gMushroom%w!", 5 },
    { EXCH_ITEM_OOTMM_CHATEAU, ITEM_OOTMM_CHATEAU, "the forbidden %yChateau Romani%w!", 200 },
    { EXCH_ITEM_OOTMM_SPRING_WATER, ITEM_OOTMM_SPRING_WATER, "fresh %cSpring Water%w!", 20 },
    { EXCH_ITEM_OOTMM_SPRING_WATER_HOT, ITEM_OOTMM_SPRING_WATER_HOT, "refreshing %cHot Spring Water%w!", 20 },
    { EXCH_ITEM_OOTMM_ZORA_EGG, ITEM_OOTMM_ZORA_EGG, "isn't this a %cZora Egg%w!", 20 },
} };

const BeggarSale* sBeggarSale = nullptr;

const BeggarSale* FindSale(int32_t exchange) {
    for (const BeggarSale& sale : kBeggarSales) {
        if (sale.Exchange == exchange) {
            return &sale;
        }
    }
    return nullptr;
}

void BuildBeggarText(uint16_t*, bool* loadFromMessageTable) {
    if (sBeggarSale == nullptr) {
        return;
    }
    CustomMessage message(std::string("Oh, ") + sBeggarSale->Line + "&I'll buy it for %r" +
                          std::to_string(sBeggarSale->Price) + " Rupees%w!&All sales final, OK?");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BuildRutoLetterText(uint16_t* textId, bool* loadFromMessageTable) {
    if (!OotmmBottles_Shared()) {
        return;
    }
    CustomMessage vanilla = CustomMessage::LoadVanillaMessageTableEntry(*textId);
    std::string text = vanilla.GetEnglish(MF_RAW);
    const size_t closing = text.rfind("\"\x09");
    if (closing == std::string::npos) {
        return;
    }
    text.insert(closing, "\x01\x05@\x06\x0c\x05"
                         "EPPS: Don't flush this letter!\x05@");
    CustomMessage message(text, vanilla.GetTextBoxType(), vanilla.GetTextBoxPosition());
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BuildKingZoraLetterText(uint16_t* textId, bool* loadFromMessageTable) {
    if (!OotmmBottles_Shared() || (OotmmItemApply_LedgerGet("native:rutoLetterUsed") & Ship::kOotmmRutoLetterUsedMm)) {
        return;
    }
    CustomMessage vanilla = CustomMessage::LoadVanillaMessageTableEntry(*textId);
    const std::string text = vanilla.GetEnglish(MF_RAW);
    const size_t page = text.find("I'll keep this letter.");
    if (page == std::string::npos) {
        return;
    }
    CustomMessage message(text.substr(0, page) + "Keep this letter and the \x05"
                                                 "Abottle\x05@\x01it was in, in case you need it.\x01"
                                                 "Take it respectfully!\x07\x40\x1c",
                          vanilla.GetTextBoxType(), vanilla.GetTextBoxPosition());
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

constexpr int kNativeBottles = 4;
constexpr const char* kChateauKey = "native:chateau";

const BottleItem* FindItem(uint8_t item) {
    if (item < ITEM_OOTMM_BOTTLE_FIRST || item >= ITEM_OOTMM_BOTTLE_MAX) {
        return nullptr;
    }
    return &kItems[item - ITEM_OOTMM_BOTTLE_FIRST];
}

uint8_t TradeSlotOf(int32_t bottle) {
    return bottle == kNativeBottles ? SLOT_TRADE_ADULT : SLOT_TRADE_CHILD;
}

std::string ExtraKey(int32_t bottle) {
    return "native:bottle" + std::to_string(bottle + 1);
}

std::string PassthroughKey(int32_t bottle) {
    return "native:bottlePassthrough" + std::to_string(bottle + 1);
}

void RefreshButtons(uint8_t slot, uint8_t item) {
    for (int button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
        if (gSaveContext.equips.cButtonSlots[button - 1] == slot) {
            gSaveContext.equips.buttonItems[button] = item;
        }
    }
}

} // namespace

extern "C" void OotmmBottles_Init(void) {
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x70F0, BuildBeggarText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x4005, BuildRutoLetterText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x401B, BuildKingZoraLetterText);
    for (const BottleItem& entry : kItems) {
        gItemAgeReqs[entry.Item] = AGE_REQ_NONE;
    }
    REGISTER_VB_SHOULD(VB_DRAW_CUSTOM_ITEM_NAME, {
        const int16_t item = static_cast<int16_t>(va_arg(args, int));
        const BottleItem* entry = FindItem(static_cast<uint8_t>(item));
        if (entry != nullptr && gPlayState != nullptr) {
            std::memcpy(gPlayState->pauseCtx.nameSegment, entry->Name, std::strlen(entry->Name) + 1);
            *should = true;
        }
    });
}

extern "C" int32_t OotmmBottles_Shared(void) {
    return OotmmSession_IsActive() && OotmmSession_GetState().GetBoolSetting("sharedBottles", false) ? 1 : 0;
}

extern "C" int32_t OotmmBottles_IsItem(uint8_t item) {
    return FindItem(item) != nullptr ? 1 : 0;
}

extern "C" int32_t OotmmBottles_IsTradeBottleSlot(int32_t slot) {
    return OotmmSession_IsActive() && (slot == SLOT_TRADE_ADULT || slot == SLOT_TRADE_CHILD) &&
           OotmmBottles_ContentOf(gSaveContext.inventory.items[slot]) != static_cast<uint32_t>(Content::None);
}

extern "C" int32_t OotmmBottles_IsAction(int32_t itemAction) {
    return itemAction >= PLAYER_IA_BOTTLE_OOTMM_MAGIC_MUSHROOM && itemAction <= PLAYER_IA_BOTTLE_OOTMM_ZORA_EGG ? 1 : 0;
}

extern "C" int32_t OotmmBottles_ItemAction(uint8_t item) {
    return FindItem(item) != nullptr ? PLAYER_IA_BOTTLE_OOTMM_MAGIC_MUSHROOM + (item - ITEM_OOTMM_BOTTLE_FIRST) : -1;
}

extern "C" int32_t OotmmBottles_IsExchange(int32_t exchangeItemId) {
    return exchangeItemId >= EXCH_ITEM_OOTMM_MAGIC_MUSHROOM && exchangeItemId <= EXCH_ITEM_OOTMM_ZORA_EGG ? 1 : 0;
}

extern "C" const char* OotmmBottles_IconPath(uint8_t item) {
    const BottleItem* entry = FindItem(item);
    return entry != nullptr ? entry->Icon : nullptr;
}

extern "C" const uint8_t* OotmmBottles_LiquidColor(int32_t itemAction) {
    if (!OotmmBottles_IsAction(itemAction)) {
        return nullptr;
    }
    return kItems[itemAction - PLAYER_IA_BOTTLE_OOTMM_MAGIC_MUSHROOM].Color;
}

extern "C" int32_t OotmmBottles_ExchangeIndex(int32_t itemAction) {
    return OotmmBottles_IsAction(itemAction) ? (itemAction - PLAYER_IA_BOTTLE_OOTMM_MAGIC_MUSHROOM) + 6 : -1;
}

extern "C" int32_t OotmmBottles_AlwaysExchange(int32_t itemAction) {
    switch (itemAction) {
        case PLAYER_IA_BOTTLE_OOTMM_MAGIC_MUSHROOM:
        case PLAYER_IA_BOTTLE_OOTMM_GOLD_DUST:
        case PLAYER_IA_BOTTLE_OOTMM_SEAHORSE:
        case PLAYER_IA_BOTTLE_OOTMM_DEKU_PRINCESS:
        case PLAYER_IA_BOTTLE_OOTMM_ZORA_EGG:
            return 1;
        default:
            return 0;
    }
}

extern "C" int32_t OotmmBottles_TalkAccepts(int32_t wanted, int32_t offered) {
    return wanted == offered || wanted == EXCH_ITEM_BLUE_FIRE ||
                   (wanted == EXCH_ITEM_ODD_MUSHROOM && offered == EXCH_ITEM_OOTMM_MAGIC_MUSHROOM)
               ? 1
               : 0;
}

extern "C" int32_t OotmmBottles_UseType(int32_t itemAction) {
    switch (itemAction) {
        case PLAYER_IA_BOTTLE_OOTMM_SPRING_WATER:
        case PLAYER_IA_BOTTLE_OOTMM_SPRING_WATER_HOT:
            return 1;
        default:
            return 0;
    }
}

extern "C" int32_t OotmmBottles_Drink(int32_t itemAction) {
    if (itemAction == PLAYER_IA_BOTTLE_OOTMM_CHATEAU) {
        OotmmItemApply_LedgerSet(kChateauKey, 1);
    }
    return 3;
}

extern "C" void OotmmBottles_Pour(PlayState* play, Player* player, float x, float y, float z) {
    if (gOotmmAquaId < 0) {
        return;
    }
    const s16 params = player->itemAction == PLAYER_IA_BOTTLE_OOTMM_SPRING_WATER_HOT ? 1 : 0;
    Actor_Spawn(&play->actorCtx, play, gOotmmAquaId, x, y, z, 0x4000, player->actor.shape.rot.y, 0, params);
}

extern "C" int32_t OotmmBottles_ChateauActive(void) {
    return OotmmSession_IsActive() && OotmmItemApply_LedgerGet(kChateauKey) != 0 ? 1 : 0;
}

extern "C" uint32_t OotmmBottles_ContentOf(uint8_t item) {
    for (const auto& [native, content] : kContents) {
        if (native == item) {
            return static_cast<uint32_t>(content);
        }
    }
    return static_cast<uint32_t>(Content::None);
}

extern "C" uint8_t OotmmBottles_ItemFor(uint32_t content) {
    for (const auto& [native, known] : kContents) {
        if (static_cast<uint32_t>(known) == content) {
            return native;
        }
    }
    return ITEM_NONE;
}

extern "C" uint32_t OotmmBottles_ReadSlot(int32_t bottle) {
    if (bottle < 0 || bottle >= Ship::kOotmmBottleCount) {
        return static_cast<uint32_t>(Content::None);
    }
    if (bottle >= kNativeBottles) {
        const uint32_t shown = OotmmBottles_ContentOf(gSaveContext.inventory.items[TradeSlotOf(bottle)]);
        if (shown != static_cast<uint32_t>(Content::None)) {
            return shown;
        }
        return OotmmItemApply_LedgerGet(ExtraKey(bottle));
    }
    const uint8_t item = gSaveContext.inventory.items[SLOT_BOTTLE_1 + bottle];
    if (item == ITEM_NONE) {
        return OotmmItemApply_LedgerGet(PassthroughKey(bottle));
    }
    return OotmmBottles_ContentOf(item);
}

extern "C" void OotmmBottles_WriteSlot(int32_t bottle, uint32_t content) {
    if (bottle < 0 || bottle >= Ship::kOotmmBottleCount) {
        return;
    }
    if (bottle >= kNativeBottles) {
        OotmmItemApply_LedgerSet(ExtraKey(bottle), content);
        const uint8_t slot = TradeSlotOf(bottle);
        const uint8_t shown = gSaveContext.inventory.items[slot];
        if (shown == ITEM_NONE || OotmmBottles_ContentOf(shown) != static_cast<uint32_t>(Content::None)) {
            const uint8_t item = OotmmBottles_ItemFor(content);
            gSaveContext.inventory.items[slot] = item;
            RefreshButtons(slot, item);
        }
        return;
    }
    const uint8_t item = OotmmBottles_ItemFor(content);
    OotmmItemApply_LedgerSet(PassthroughKey(bottle), item == ITEM_NONE ? content : 0);
    const uint8_t slot = static_cast<uint8_t>(SLOT_BOTTLE_1 + bottle);
    gSaveContext.inventory.items[slot] = item;
    RefreshButtons(slot, item);
}

extern "C" int32_t OotmmBottles_GiveNew(uint8_t item) {
    const uint32_t content = OotmmBottles_ContentOf(item);
    if (content == static_cast<uint32_t>(Content::None)) {
        return 0;
    }
    for (int32_t bottle = 0; bottle < Ship::kOotmmBottleCount; bottle++) {
        if (OotmmBottles_ReadSlot(bottle) == static_cast<uint32_t>(Content::None)) {
            OotmmBottles_WriteSlot(bottle, content);
            return 1;
        }
    }
    return 0;
}

extern "C" int32_t OotmmBottles_FillEmpty(uint8_t item) {
    const uint32_t content = OotmmBottles_ContentOf(item == ITEM_MILK ? ITEM_MILK_BOTTLE : item);
    if (content == static_cast<uint32_t>(Content::None)) {
        return 0;
    }
    for (int32_t bottle = 0; bottle < Ship::kOotmmBottleCount; bottle++) {
        if (OotmmBottles_ReadSlot(bottle) == static_cast<uint32_t>(Content::Empty)) {
            OotmmBottles_WriteSlot(bottle, content);
            return 1;
        }
    }
    return 0;
}

extern "C" int32_t OotmmBottles_Holds(uint8_t item) {
    const uint32_t content = OotmmBottles_ContentOf(item);
    for (int32_t bottle = 0; bottle < Ship::kOotmmBottleCount; bottle++) {
        if (OotmmBottles_ReadSlot(bottle) == content) {
            return 1;
        }
    }
    return 0;
}

extern "C" int32_t OotmmBottles_ConsumeFairy(PlayState* play) {
    int32_t selected = -1;
    for (int32_t bottle = 0; bottle < Ship::kOotmmBottleCount; bottle++) {
        if (OotmmBottles_ReadSlot(bottle) == static_cast<uint32_t>(Content::Fairy)) {
            selected = bottle;
            break;
        }
    }
    if (selected < 0) {
        return 0;
    }
    for (int button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
        const int32_t bottle = gSaveContext.equips.cButtonSlots[button - 1] - SLOT_BOTTLE_1;
        if (gSaveContext.equips.buttonItems[button] == ITEM_FAIRY &&
            OotmmBottles_ReadSlot(bottle) == static_cast<uint32_t>(Content::Fairy)) {
            selected = bottle;
            break;
        }
    }
    OotmmBottles_WriteSlot(selected, static_cast<uint32_t>(Content::Empty));
    for (int button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
        if (gSaveContext.equips.cButtonSlots[button - 1] == SLOT_BOTTLE_1 + selected &&
            gSaveContext.equips.buttonItems[button] == ITEM_BOTTLE) {
            Interface_LoadItemIcon1(play, button);
        }
    }
    return 1;
}

extern "C" void OotmmBottles_GiveRutoLetter(PlayState* play) {
    if (!OotmmSession_IsActive()) {
        Inventory_ReplaceItem(play, ITEM_LETTER_RUTO, ITEM_BOTTLE);
        return;
    }
    const uint32_t used = OotmmItemApply_LedgerGet("native:rutoLetterUsed") | Ship::kOotmmRutoLetterUsedOot;
    OotmmItemApply_LedgerSet("native:rutoLetterUsed", used);
    const uint32_t both = Ship::kOotmmRutoLetterUsedOot | Ship::kOotmmRutoLetterUsedMm;
    if (OotmmBottles_Shared() && (used & both) != both) {
        return;
    }
    for (int32_t bottle = 0; bottle < Ship::kOotmmBottleCount; bottle++) {
        if (OotmmBottles_ReadSlot(bottle) == static_cast<uint32_t>(Content::RutoLetter)) {
            OotmmBottles_WriteSlot(bottle, static_cast<uint32_t>(Content::Empty));
            return;
        }
    }
}

extern "C" uint8_t OotmmBottles_TradeSlotBottle(uint8_t slot) {
    if (!OotmmSession_IsActive() || (slot != SLOT_TRADE_ADULT && slot != SLOT_TRADE_CHILD)) {
        return ITEM_NONE;
    }
    const int32_t bottle = slot == SLOT_TRADE_ADULT ? kNativeBottles : kNativeBottles + 1;
    const uint32_t content = OotmmBottles_ReadSlot(bottle);
    if (OotmmBottles_ContentOf(gSaveContext.inventory.items[slot]) != static_cast<uint32_t>(Content::None)) {
        OotmmItemApply_LedgerSet(ExtraKey(bottle), content);
    }
    return OotmmBottles_ItemFor(content);
}

extern "C" int32_t OotmmBottles_HasSpecific(uint8_t item) {
    return OotmmSession_IsActive() ? OotmmBottles_Holds(item) : Inventory_HasSpecificBottle(item);
}

extern "C" int32_t OotmmBottles_BeggarHasSellable(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    for (const BeggarSale& sale : kBeggarSales) {
        if (OotmmBottles_Holds(sale.Item)) {
            return 1;
        }
    }
    return 0;
}

extern "C" int32_t OotmmBottles_BeggarRefuses(int32_t exchangeItemId) {
    return exchangeItemId == EXCH_ITEM_OOTMM_DEKU_PRINCESS || exchangeItemId == EXCH_ITEM_OOTMM_SEAHORSE ||
                   exchangeItemId == EXCH_ITEM_OOTMM_GOLD_DUST
               ? 1
               : 0;
}

extern "C" int32_t OotmmBottles_BeggarOffer(int32_t exchangeItemId) {
    sBeggarSale = OotmmSession_IsActive() ? FindSale(exchangeItemId) : nullptr;
    return sBeggarSale != nullptr ? 1 : 0;
}

extern "C" int32_t OotmmBottles_BeggarPayout(int16_t vanillaReward) {
    const BeggarSale* sale = sBeggarSale;
    sBeggarSale = nullptr;
    return sale != nullptr ? sale->Price : vanillaReward;
}
