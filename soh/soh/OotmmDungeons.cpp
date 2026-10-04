#include "OotmmDungeons.h"

#include "OotmmSession.h"
#include "OotmmTriforce.h"

#include <libultraship/bridge/OotmmProgress.h>

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

} // namespace

extern "C" int OotmmDungeons_ChestGameKeepsProgress(void) {
    return OotmmSession_IsActive() && OotmmDungeons_MaxSmallKeys(SCENE_TREASURE_BOX_SHOP) != 0 ? 1 : 0;
}

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

extern "C" const char* OotmmDungeons_CodeForScene(int sceneNum) {
    for (const Dungeon& dungeon : kDungeons) {
        if (dungeon.Scene == sceneNum) {
            return dungeon.Code.data();
        }
    }
    return nullptr;
}

extern "C" int OotmmDungeons_MaxSmallKeys(int sceneNum) {
    const char* code = OotmmDungeons_CodeForScene(sceneNum);
    if (code == nullptr) {
        return 0;
    }
    return static_cast<int>(Ship::OotmmMaxSmallKeys(OotmmSession_GetState(), Ship::OotmmGame::Oot, code));
}

extern "C" int OotmmDungeons_SmallKeysRemoved(int sceneNum) {
    return SmallKeyShuffle() == "removed" && sceneNum != SCENE_GERUDOS_FORTRESS && sceneNum != SCENE_TREASURE_BOX_SHOP;
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
    return sceneNum == SCENE_WATER_TEMPLE && switchFlag == 0x15 && OotmmSession_IsSceneMasterQuest(sceneNum) == 0;
}
