#include "OotmmMasterSword.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmItemApply.h"
#include "OotmmNpcChecks.h"
#include "OotmmSession.h"

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
}

namespace {

constexpr u16 kDefaultEquipment = (EQUIP_VALUE_TUNIC_KOKIRI << (EQUIP_TYPE_TUNIC * 4)) |
                                  (EQUIP_VALUE_BOOTS_KOKIRI << (EQUIP_TYPE_BOOTS * 4));

bool MustStartWithMasterSword() {
    const auto& state = OotmmSession_GetState();
    if (state.GetStringSetting("startingAge", "child") != "adult") {
        return false;
    }
    if (!state.GetBoolSetting("swordlessAdult", false)) {
        return true;
    }
    return !state.GetBoolSetting("shuffleMasterSword", false) && state.GetBoolSetting("timeTravelSword", false);
}

bool FoundMasterSword() {
    return OotmmItemApply_FoundMasterSword() || MustStartWithMasterSword();
}

bool IsBottled(u8 item) {
    return (item >= ITEM_BOTTLE && item <= ITEM_POE) || (item >= ITEM_WEIRD_EGG && item <= ITEM_CLAIM_CHECK);
}

void EquipMasterSword() {
    gSaveContext.equips.buttonItems[0] = ITEM_SWORD_MASTER;
    Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_MASTER);
    Flags_UnsetInfTable(INFTABLE_SWORDLESS);
}

void FixMasterSword() {
    if (!OotmmSession_IsActive() || !FoundMasterSword()) {
        return;
    }
    gSaveContext.inventory.equipment |= OWNED_EQUIP_FLAG(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER);
    if (OotmmSession_BoolSetting("swordlessAdult") || !LINK_IS_ADULT ||
        gSaveContext.equips.buttonItems[0] != ITEM_NONE) {
        return;
    }
    EquipMasterSword();
}

} // namespace

extern "C" int32_t OotmmMasterSword_CanTimeTravel(void) {
    return !OotmmSession_BoolSetting("timeTravelSword") ||
           CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER) || FoundMasterSword();
}

extern "C" int32_t OotmmMasterSword_PedestalReady(Actor* actor, PlayState* play) {
    if (!OotmmSession_IsActive()) {
        return !LINK_IS_ADULT || Flags_GetEventChkInf(EVENTCHKINF_LEARNED_PRELUDE_OF_LIGHT);
    }
    if (LINK_IS_ADULT && !Flags_GetEventChkInf(EVENTCHKINF_PULLED_MASTER_SWORD_FROM_PEDESTAL)) {
        if (OotmmNpcChecks_TryCollectFreestanding(actor, play, "OOT_LIGHT_MEDALLION", 200.0f, 50.0f)) {
            Flags_SetEventChkInf(EVENTCHKINF_PULLED_MASTER_SWORD_FROM_PEDESTAL);
        }
        return 0;
    }
    return 1;
}

extern "C" int32_t OotmmMasterSword_SwapAgeEquipment(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    ItemEquips* previous = LINK_IS_ADULT ? &gSaveContext.adultEquips : &gSaveContext.childEquips;
    ItemEquips* next = LINK_IS_ADULT ? &gSaveContext.childEquips : &gSaveContext.adultEquips;
    *previous = gSaveContext.equips;
    if (Flags_GetInfTable(INFTABLE_SWORDLESS)) {
        previous->buttonItems[0] = ITEM_NONE;
    }
    gSaveContext.equips = *next;
    for (size_t i = 1; i < ARRAY_COUNT(gSaveContext.equips.buttonItems); i++) {
        const u8 slot = gSaveContext.equips.cButtonSlots[i - 1];
        if (IsBottled(gSaveContext.equips.buttonItems[i]) && slot < ARRAY_COUNT(gSaveContext.inventory.items)) {
            gSaveContext.equips.buttonItems[i] = gSaveContext.inventory.items[slot];
        }
    }
    if (gSaveContext.equips.buttonItems[0] == ITEM_NONE) {
        Flags_SetInfTable(INFTABLE_SWORDLESS);
    } else {
        Flags_UnsetInfTable(INFTABLE_SWORDLESS);
    }
    const s32 shield = CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD);
    if (shield != EQUIP_VALUE_SHIELD_NONE && !CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, shield - 1)) {
        Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_NONE);
    }
    return 1;
}

void OotmmMasterSword_InitializeNewSave() {
    gSaveContext.childEquips.buttonItems[0] = ITEM_NONE;
    gSaveContext.childEquips.equipment = kDefaultEquipment;
    gSaveContext.adultEquips.buttonItems[0] = ITEM_NONE;
    gSaveContext.adultEquips.equipment = kDefaultEquipment;
    if (LINK_IS_ADULT && MustStartWithMasterSword()) {
        gSaveContext.inventory.equipment |= OWNED_EQUIP_FLAG(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER);
        EquipMasterSword();
    }
}

void OotmmMasterSword_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { FixMasterSword(); });
}
