#include "OotmmDungeons.h"

#include "OotmmSession.h"
#include "OotmmTriforce.h"

#include <array>
#include <string>
#include <string_view>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
}

namespace {

struct Dungeon {
    std::string_view Code;
    int16_t Scene;
};

constexpr std::array<Dungeon, 14> kDungeons = { {
    { "DT", SCENE_DEKU_TREE },
    { "DC", SCENE_DODONGOS_CAVERN },
    { "JJ", SCENE_JABU_JABU },
    { "FOREST", SCENE_FOREST_TEMPLE },
    { "FIRE", SCENE_FIRE_TEMPLE },
    { "WATER", SCENE_WATER_TEMPLE },
    { "SPIRIT", SCENE_SPIRIT_TEMPLE },
    { "SHADOW", SCENE_SHADOW_TEMPLE },
    { "BOTW", SCENE_BOTTOM_OF_THE_WELL },
    { "IC", SCENE_ICE_CAVERN },
    { "GTG", SCENE_GERUDO_TRAINING_GROUND },
    { "GF", SCENE_THIEVES_HIDEOUT },
    { "GANON", SCENE_INSIDE_GANONS_CASTLE },
    { "TCG", SCENE_TREASURE_BOX_SHOP },
} };

std::string SmallKeyShuffle() {
    return OotmmSession_GetState().GetStringSetting("smallKeyShuffleOot", "ownDungeon");
}

int MainDungeonSmallKeys(int sceneNum) {
    const bool mq = OotmmSession_IsSceneMasterQuest(sceneNum) != 0;
    switch (sceneNum) {
        case SCENE_FOREST_TEMPLE:
            return mq ? 6 : 5;
        case SCENE_FIRE_TEMPLE:
            return mq ? 5 : (SmallKeyShuffle() == "anywhere" ? 8 : 7);
        case SCENE_WATER_TEMPLE:
            return mq ? 2 : 5;
        case SCENE_SPIRIT_TEMPLE:
            return mq ? 7 : 5;
        case SCENE_SHADOW_TEMPLE:
            return mq ? 6 : 5;
        case SCENE_BOTTOM_OF_THE_WELL:
            return mq ? 2 : 3;
        case SCENE_GERUDO_TRAINING_GROUND:
            return mq ? 3 : 9;
        default:
            return 0;
    }
}

int HideoutSmallKeys() {
    const std::string fortress = OotmmSession_GetState().GetStringSetting("gerudoFortress", "vanilla");
    if (fortress == "open") {
        return 0;
    }
    return fortress == "single" ? 1 : GERUDO_FORTRESS_SMALL_KEY_MAX;
}

int ChestGameSmallKeys() {
    return OotmmSession_GetState().GetStringSetting("smallKeyShuffleChestGame", "vanilla") == "vanilla"
               ? 0
               : TREASURE_GAME_SMALL_KEY_MAX;
}

} // namespace

extern "C" int OotmmDungeons_SceneForCode(const char* code) {
    if (code == nullptr) {
        return -1;
    }
    const std::string_view wanted = code;
    for (const Dungeon& dungeon : kDungeons) {
        if (dungeon.Code == wanted) {
            return dungeon.Scene;
        }
    }
    return -1;
}

extern "C" int OotmmDungeons_MaxSmallKeys(int sceneNum) {
    switch (sceneNum) {
        case SCENE_INSIDE_GANONS_CASTLE:
            return OotmmSession_IsSceneMasterQuest(sceneNum) != 0 ? 3 : 2;
        case SCENE_THIEVES_HIDEOUT:
            return HideoutSmallKeys();
        case SCENE_TREASURE_BOX_SHOP:
            return ChestGameSmallKeys();
        default:
            return SmallKeyShuffle() == "removed" ? 0 : MainDungeonSmallKeys(sceneNum);
    }
}

extern "C" int OotmmDungeons_SmallKeysRemoved(int sceneNum) {
    return SmallKeyShuffle() == "removed" && sceneNum != SCENE_GERUDOS_FORTRESS &&
           sceneNum != SCENE_TREASURE_BOX_SHOP;
}

extern "C" int OotmmDungeons_BossDoorIsOpen(int sceneNum) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const auto& state = OotmmSession_GetState();
    if (sceneNum != SCENE_GANONS_TOWER) {
        return state.GetStringSetting("bossKeyShuffleOot", "") == "removed";
    }
    if (OotmmTriforce_Active()) {
        return OotmmTriforce_HasWon();
    }
    return state.GetStringSetting("ganonBossKey", "") == "removed";
}

extern "C" int OotmmDungeons_DoorIsForcedOpen(int sceneNum, int switchFlag) {
    if (sceneNum == SCENE_FIRE_TEMPLE && switchFlag == 0x17) {
        return SmallKeyShuffle() != "anywhere" && OotmmSession_IsSceneMasterQuest(sceneNum) == 0;
    }
    return sceneNum == SCENE_WATER_TEMPLE && switchFlag == 0x15 &&
           OotmmSession_IsSceneMasterQuest(sceneNum) == 0;
}
