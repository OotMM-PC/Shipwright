#include "OotmmRustyKeys.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

#include <array>
#include <cstdint>

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64.h"
}

namespace {

enum RustyKeyId {
    KEY_TREASURE_CHEST_GAME,
    KEY_GUARD_HOUSE,
    KEY_HYRULE_CASTLE,
    KEY_DOG_LADY_HOUSE,
    KEY_BACK_ALLEY_HOUSE,
    KEY_BOMBCHU_SHOP,
    KEY_MASK_SHOP,
    KEY_CHILD_BAZAAR,
    KEY_CHILD_POTION_SHOP,
    KEY_CHILD_SHOOTING_GALLERY,
    KEY_BOMBCHU_BOWLING,
    KEY_LABORATORY,
    KEY_FISHING_POND,
    KEY_SILO,
    KEY_RANCH_STABLE,
    KEY_RANCH_HOUSE,
    KEY_RANCH_HOUSE_ROOM,
    KEY_GRAVEYARD,
    KEY_WINDMILL,
    KEY_IMPA_HOUSE,
    KEY_CARPENTER_HOUSE,
    KEY_GRANNY_POTION_SHOP,
    KEY_ADULT_SHOOTING_GALLERY,
    KEY_SKULLTULA_HOUSE,
    KEY_ADULT_BAZAAR,
    KEY_ADULT_POTION_SHOP,
    KEY_ADULT_POTION_SHOP_BACK,
    KEY_MAX,
};

constexpr std::array<const char*, KEY_MAX> kItemIds = {
    "OOT_RUSTY_KEY_TREASURE_CHEST_GAME", "OOT_RUSTY_KEY_GUARD_HOUSE",
    "OOT_RUSTY_KEY_HYRULE_CASTLE", "OOT_RUSTY_KEY_DOG_LADY_HOUSE",
    "OOT_RUSTY_KEY_BACK_ALLEY_HOUSE", "OOT_RUSTY_KEY_BOMBCHU_SHOP",
    "OOT_RUSTY_KEY_MASK_SHOP", "OOT_RUSTY_KEY_CHILD_BAZAAR",
    "OOT_RUSTY_KEY_CHILD_POTION_SHOP", "OOT_RUSTY_KEY_CHILD_SHOOTING_GALLERY",
    "OOT_RUSTY_KEY_BOMBCHU_BOWLING", "OOT_RUSTY_KEY_LABORATORY",
    "OOT_RUSTY_KEY_FISHING_POND", "OOT_RUSTY_KEY_SILO", "OOT_RUSTY_KEY_RANCH_STABLE",
    "OOT_RUSTY_KEY_RANCH_HOUSE", "OOT_RUSTY_KEY_RANCH_HOUSE_ROOM", "OOT_RUSTY_KEY_GRAVEYARD",
    "OOT_RUSTY_KEY_WINDMILL", "OOT_RUSTY_KEY_IMPA_HOUSE", "OOT_RUSTY_KEY_CARPENTER_HOUSE",
    "OOT_RUSTY_KEY_GRANNY_POTION_SHOP", "OOT_RUSTY_KEY_ADULT_SHOOTING_GALLERY",
    "OOT_RUSTY_KEY_SKULLTULA_HOUSE", "OOT_RUSTY_KEY_ADULT_BAZAAR",
    "OOT_RUSTY_KEY_ADULT_POTION_SHOP", "OOT_RUSTY_KEY_ADULT_POTION_SHOP_BACK",
};

int ResolveDoor(int scene, int transitionId, int spawn) {
    switch (scene) {
        case SCENE_MARKET_ENTRANCE_DAY:
        case SCENE_MARKET_ENTRANCE_NIGHT:
        case SCENE_MARKET_ENTRANCE_RUINS:
        case SCENE_MARKET_GUARD_HOUSE:
            return KEY_GUARD_HOUSE;
        case SCENE_BACK_ALLEY_DAY:
        case SCENE_BACK_ALLEY_NIGHT:
            if (transitionId == 0) return KEY_DOG_LADY_HOUSE;
            if (transitionId == 1) return KEY_BACK_ALLEY_HOUSE;
            if (transitionId == 2) return KEY_BOMBCHU_SHOP;
            break;
        case SCENE_DOG_LADY_HOUSE:
            return KEY_DOG_LADY_HOUSE;
        case SCENE_BACK_ALLEY_HOUSE:
            return KEY_BACK_ALLEY_HOUSE;
        case SCENE_HYRULE_CASTLE:
            return KEY_HYRULE_CASTLE;
        case SCENE_TREASURE_BOX_SHOP:
            return KEY_TREASURE_CHEST_GAME;
        case SCENE_MARKET_DAY:
        case SCENE_MARKET_NIGHT:
            if (transitionId == 0) return KEY_TREASURE_CHEST_GAME;
            if (transitionId == 1) return KEY_CHILD_POTION_SHOP;
            if (transitionId == 2) return KEY_CHILD_SHOOTING_GALLERY;
            if (transitionId == 3) return KEY_MASK_SHOP;
            if (transitionId == 4) return KEY_CHILD_BAZAAR;
            if (transitionId == 5) return KEY_BOMBCHU_BOWLING;
            break;
        case SCENE_BOMBCHU_BOWLING_ALLEY:
            return KEY_BOMBCHU_BOWLING;
        case SCENE_SHOOTING_GALLERY:
            if (spawn == 0) return KEY_ADULT_SHOOTING_GALLERY;
            if (spawn == 1) return KEY_CHILD_SHOOTING_GALLERY;
            break;
        case SCENE_LAKE_HYLIA:
            if (transitionId == 0) return KEY_LABORATORY;
            if (transitionId == 1) return KEY_FISHING_POND;
            break;
        case SCENE_LAKESIDE_LABORATORY:
            return KEY_LABORATORY;
        case SCENE_FISHING_POND:
            return KEY_FISHING_POND;
        case SCENE_LON_LON_RANCH:
            if (transitionId == 0) return KEY_SILO;
            if (transitionId == 1) return KEY_RANCH_STABLE;
            if (transitionId == 2) return KEY_RANCH_HOUSE;
            break;
        case SCENE_STABLE:
            return KEY_RANCH_STABLE;
        case SCENE_LON_LON_BUILDINGS:
            if (transitionId == 0) return KEY_SILO;
            if (transitionId == 1) return KEY_RANCH_HOUSE;
            if (transitionId == 2) return KEY_RANCH_HOUSE_ROOM;
            break;
        case SCENE_GRAVEYARD:
            return KEY_GRAVEYARD;
        case SCENE_KAKARIKO_VILLAGE:
            if (transitionId == 0) return KEY_WINDMILL;
            if (transitionId == 1) return KEY_IMPA_HOUSE;
            if (transitionId == 2) return KEY_CARPENTER_HOUSE;
            if (transitionId == 3) return KEY_GRANNY_POTION_SHOP;
            if (transitionId == 4) return KEY_ADULT_SHOOTING_GALLERY;
            if (transitionId == 5) return KEY_SKULLTULA_HOUSE;
            if (transitionId == 6) return KEY_ADULT_BAZAAR;
            if (transitionId == 7) return KEY_ADULT_POTION_SHOP;
            if (transitionId == 8) return KEY_ADULT_POTION_SHOP_BACK;
            break;
        case SCENE_WINDMILL_AND_DAMPES_GRAVE:
            return KEY_WINDMILL;
        case SCENE_HOUSE_OF_SKULLTULA:
            return KEY_SKULLTULA_HOUSE;
        case SCENE_KAKARIKO_CENTER_GUEST_HOUSE:
            return KEY_CARPENTER_HOUSE;
        case SCENE_IMPAS_HOUSE:
            return KEY_IMPA_HOUSE;
        case SCENE_GRAVEKEEPERS_HUT:
            return KEY_GRAVEYARD;
        case SCENE_POTION_SHOP_GRANNY:
            return KEY_GRANNY_POTION_SHOP;
    }
    return -1;
}

} // namespace

extern "C" int OotmmRustyDoorLocked(PlayState* play, Actor* actor) {
    if (!OotmmSession_IsActive() || !OotmmSession_GetState().GetBoolSetting("rustyKeysOot", false) ||
        play == nullptr || actor == nullptr) {
        return 0;
    }
    const int key = ResolveDoor(play->sceneNum, static_cast<uint16_t>(actor->params) >> 10, play->curSpawn);
    return key >= 0 && !OotmmIpc_GetInventory().Has(kItemIds[key]);
}
