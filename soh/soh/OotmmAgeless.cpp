#include "OotmmAgeless.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

#include <initializer_list>

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
}

namespace {

struct AgelessGroup {
    const char* Setting;
    std::initializer_list<int> Slots;
    std::initializer_list<int> Items;
    int EquipType;
};

constexpr int kNoEquip = -1;

const AgelessGroup kGroups[] = {
    { "agelessSwords", {}, { ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER, ITEM_SWORD_BGS, ITEM_SWORD_KNIFE }, EQUIP_TYPE_SWORD },
    { "agelessShields", {}, { ITEM_SHIELD_DEKU, ITEM_SHIELD_HYLIAN, ITEM_SHIELD_MIRROR }, EQUIP_TYPE_SHIELD },
    { "agelessTunics", {}, { ITEM_TUNIC_KOKIRI, ITEM_TUNIC_GORON, ITEM_TUNIC_ZORA }, EQUIP_TYPE_TUNIC },
    { "agelessBoots", {}, { ITEM_BOOTS_KOKIRI, ITEM_BOOTS_IRON, ITEM_BOOTS_HOVER }, EQUIP_TYPE_BOOTS },
    { "agelessSticks", { SLOT_STICK }, { ITEM_STICK }, kNoEquip },
    { "agelessBoomerang", { SLOT_BOOMERANG }, { ITEM_BOOMERANG }, kNoEquip },
    { "agelessHammer", { SLOT_HAMMER }, { ITEM_HAMMER }, kNoEquip },
    { "agelessHookshot", { SLOT_HOOKSHOT }, { ITEM_HOOKSHOT, ITEM_LONGSHOT }, kNoEquip },
    { "agelessSlingshot", { SLOT_SLINGSHOT }, { ITEM_SLINGSHOT }, kNoEquip },
    { "agelessBow", { SLOT_BOW, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT },
      { ITEM_BOW, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT }, kNoEquip },
    { "agelessChildTrade", { SLOT_TRADE_CHILD },
      { ITEM_WEIRD_EGG, ITEM_CHICKEN, ITEM_LETTER_ZELDA, ITEM_MASK_KEATON, ITEM_MASK_SKULL, ITEM_MASK_SPOOKY,
        ITEM_MASK_BUNNY, ITEM_MASK_GORON, ITEM_MASK_ZORA, ITEM_MASK_GERUDO, ITEM_MASK_TRUTH },
      kNoEquip },
};

void ApplyAgeTables() {
    if (!OotmmSession_IsActive()) {
        return;
    }
    for (const auto& group : kGroups) {
        if (!OotmmSession_BoolSetting(group.Setting)) {
            continue;
        }
        for (const int slot : group.Slots) {
            gSlotAgeReqs[slot] = AGE_REQ_NONE;
        }
        for (const int item : group.Items) {
            gItemAgeReqs[item] = AGE_REQ_NONE;
        }
        if (group.EquipType != kNoEquip) {
            for (int value = 1; value < 4; value++) {
                gEquipAgeReqs[group.EquipType][value] = AGE_REQ_NONE;
            }
        }
    }
}

int16_t ArrowSeedDrop(int size) {
    static constexpr int16_t kDrops[] = { ITEM00_ARROWS_SMALL, ITEM00_ARROWS_MEDIUM, ITEM00_ARROWS_LARGE,
                                          ITEM00_SEEDS,        ITEM00_SEEDS,         ITEM00_SEEDS };
    const bool arrows = !(LINK_IS_CHILD && !OotmmSession_BoolSetting("agelessBow")) && CUR_UPG_VALUE(UPG_QUIVER) != 0;
    const bool seeds =
        !(LINK_IS_ADULT && !OotmmSession_BoolSetting("agelessSlingshot")) && CUR_UPG_VALUE(UPG_BULLET_BAG) != 0;
    if (!arrows && !seeds) {
        return -1;
    }
    const int family = !seeds ? 0 : !arrows ? 1 : (Rand_ZeroOne() < 0.5f ? 0 : 1);
    return kDrops[family * 3 + size];
}

} // namespace

extern "C" int32_t OotmmAgeless_Strength(void) {
    return OotmmSession_BoolSetting("agelessStrength");
}

extern "C" int32_t OotmmAgeless_ChildTrade(void) {
    return OotmmSession_BoolSetting("agelessChildTrade");
}

extern "C" int32_t OotmmAgeless_EquipmentVisible(void) {
    return CVarGetInteger(CVAR_ENHANCEMENT("EquipmentAlwaysVisible"), 0) || OotmmSession_IsActive();
}

extern "C" int16_t OotmmAgeless_FixDrop(int16_t dropId) {
    switch (dropId) {
        case ITEM00_STICK:
            return LINK_IS_ADULT && !OotmmSession_BoolSetting("agelessSticks") ? ITEM00_RUPEE_GREEN : dropId;
        case ITEM00_ARROWS_SMALL:
        case ITEM00_SEEDS:
            return ArrowSeedDrop(0);
        case ITEM00_ARROWS_MEDIUM:
            return ArrowSeedDrop(1);
        case ITEM00_ARROWS_LARGE:
            return ArrowSeedDrop(2);
        default:
            return dropId;
    }
}

void OotmmAgeless_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneSpawnActors>([]() { ApplyAgeTables(); });
}
