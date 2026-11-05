#include "OotmmSilverRupees.h"

#include "OotmmIpc.h"

#include <libultraship/bridge/OotmmItemGrant.h>
#include "OotmmSession.h"

#include <cstdint>
#include <string>
#include <string_view>

extern "C" {
extern PlayState* gPlayState;
#include "functions.h"
#include "variables.h"
#include "z64.h"
}


namespace {

constexpr int16_t kAnyRoom = -1;

struct Puzzle {
    int16_t scene;
    int16_t room;
    bool masterQuest;
    const char* suffix;
    uint8_t base;
    int8_t flag;
};

constexpr Puzzle kPuzzles[] = {
    { SCENE_BOTTOM_OF_THE_WELL, kAnyRoom, false, "BOTW", 0x05, 0x1f },
    { SCENE_SPIRIT_TEMPLE, 0x00, false, "SPIRIT_CHILD", 0x0a, 0x05 },
    { SCENE_SPIRIT_TEMPLE, 0x02, false, "SPIRIT_CHILD", 0x0a, 0x05 },
    { SCENE_SPIRIT_TEMPLE, 0x08, false, "SPIRIT_SUN", 0x0f, 0x0a },
    { SCENE_SPIRIT_TEMPLE, 0x17, false, "SPIRIT_SUN", 0x0f, 0x0a },
    { SCENE_SPIRIT_TEMPLE, 0x0d, false, "SPIRIT_BOULDERS", 0x14, 0x02 },
    { SCENE_SHADOW_TEMPLE, 0x06, false, "SHADOW_SCYTHE", 0x19, 0x01 },
    { SCENE_SHADOW_TEMPLE, 0x09, false, "SHADOW_PIT", 0x28, 0x09 },
    { SCENE_SHADOW_TEMPLE, 0x0b, false, "SHADOW_SPIKES", 0x2d, 0x08 },
    { SCENE_ICE_CAVERN, 0x03, false, "IC_SCYTHE", 0x37, 0x08 },
    { SCENE_ICE_CAVERN, 0x05, false, "IC_BLOCK", 0x3c, 0x09 },
    { SCENE_GERUDO_TRAINING_GROUND, 0x02, false, "GTG_SLOPES", 0x41, 0x1c },
    { SCENE_GERUDO_TRAINING_GROUND, 0x06, false, "GTG_LAVA", 0x46, 0x0c },
    { SCENE_GERUDO_TRAINING_GROUND, 0x09, false, "GTG_WATER", 0x4c, 0x1b },
    { SCENE_INSIDE_GANONS_CASTLE, 0x0c, false, "GANON_SPIRIT", 0x51, 0x0b },
    { SCENE_INSIDE_GANONS_CASTLE, 0x11, false, "GANON_SPIRIT", 0x51, 0x0b },
    { SCENE_INSIDE_GANONS_CASTLE, 0x03, false, "GANON_LIGHT", 0x56, 0x12 },
    { SCENE_INSIDE_GANONS_CASTLE, 0x08, false, "GANON_LIGHT", 0x56, 0x12 },
    { SCENE_INSIDE_GANONS_CASTLE, 0x0e, false, "GANON_FIRE", 0x5b, 0x09 },
    { SCENE_INSIDE_GANONS_CASTLE, 0x06, false, "GANON_FOREST", 0x60, 0x0e },

    { SCENE_DODONGOS_CAVERN, kAnyRoom, true, "DC", 0x00, -1 },
    { SCENE_SPIRIT_TEMPLE, 0x00, true, "SPIRIT_LOBBY", 0x0a, -1 },
    { SCENE_SPIRIT_TEMPLE, 0x02, true, "SPIRIT_LOBBY", 0x0a, -1 },
    { SCENE_SPIRIT_TEMPLE, 0x08, true, "SPIRIT_ADULT", 0x0f, 0x00 },
    { SCENE_SPIRIT_TEMPLE, 0x17, true, "SPIRIT_ADULT", 0x0f, 0x00 },
    { SCENE_SHADOW_TEMPLE, 0x06, true, "SHADOW_SCYTHE", 0x19, 0x01 },
    { SCENE_SHADOW_TEMPLE, 0x10, true, "SHADOW_BLADES", 0x1e, 0x03 },
    { SCENE_SHADOW_TEMPLE, 0x09, true, "SHADOW_PIT", 0x28, 0x11 },
    { SCENE_SHADOW_TEMPLE, 0x0b, true, "SHADOW_SPIKES", 0x2d, 0x08 },
    { SCENE_GERUDO_TRAINING_GROUND, 0x02, true, "GTG_SLOPES", 0x41, 0x1c },
    { SCENE_GERUDO_TRAINING_GROUND, 0x06, true, "GTG_LAVA", 0x46, 0x0c },
    { SCENE_GERUDO_TRAINING_GROUND, 0x09, true, "GTG_WATER", 0x4c, 0x1b },
    { SCENE_INSIDE_GANONS_CASTLE, 0x0c, true, "GANON_SHADOW", 0x51, 0x0b },
    { SCENE_INSIDE_GANONS_CASTLE, 0x11, true, "GANON_SHADOW", 0x51, 0x0b },
    { SCENE_INSIDE_GANONS_CASTLE, 0x03, true, "GANON_WATER", 0x56, 0x02 },
    { SCENE_INSIDE_GANONS_CASTLE, 0x08, true, "GANON_WATER", 0x56, 0x02 },
    { SCENE_INSIDE_GANONS_CASTLE, 0x0e, true, "GANON_FIRE", 0x5b, 0x01 },
};

bool ShuffleActive() {
    return OotmmSession_IsActive() &&
           OotmmSession_GetState().GetStringSetting("silverRupeeShuffle", "vanilla") != "vanilla";
}

bool OwnsMagicalRupee() {
    return OotmmIpc_GetInventory().Has("OOT_RUPEE_MAGICAL");
}

bool Satisfied(const Puzzle& puzzle, bool masterQuest) {
    const auto& inventory = OotmmIpc_GetInventory();
    const std::string suffix(puzzle.suffix);
    const uint32_t required = Ship::OotmmSilverRupees::Required(puzzle.suffix, masterQuest);
    return inventory.Has("OOT_POUCH_SILVER_" + suffix) ||
           (required != 0 && inventory.Count("OOT_RUPEE_SILVER_" + suffix) >= required);
}

const Puzzle* Find(int16_t scene, int16_t room, bool masterQuest) {
    for (const Puzzle& puzzle : kPuzzles) {
        if (puzzle.scene != scene || puzzle.masterQuest != masterQuest) {
            continue;
        }
        if (puzzle.room == kAnyRoom || puzzle.room == room) {
            return &puzzle;
        }
    }
    return nullptr;
}

const Puzzle* FindEitherQuest(int16_t scene, int16_t room) {
    for (const Puzzle& puzzle : kPuzzles) {
        if (puzzle.scene == scene && (puzzle.room == kAnyRoom || puzzle.room == room)) {
            return &puzzle;
        }
    }
    return nullptr;
}

} // namespace

extern "C" int OotmmSilverRupeesSolved(PlayState* play, int room) {
    if (play == nullptr || !ShuffleActive()) {
        return 0;
    }
    const bool masterQuest = OotmmSession_IsSceneMasterQuest(play->sceneNum) != 0;
    const Puzzle* puzzle = Find(play->sceneNum, static_cast<int16_t>(room), masterQuest);
    if (puzzle == nullptr) {
        return 0;
    }
    return OwnsMagicalRupee() || Satisfied(*puzzle, masterQuest);
}

extern "C" int OotmmSilverRupeesPuzzleIds(int scene, int room, int* base, int* width) {
    if (base == nullptr || width == nullptr || !ShuffleActive()) {
        return 0;
    }
    const Puzzle* puzzle = FindEitherQuest(static_cast<int16_t>(scene), static_cast<int16_t>(room));
    if (puzzle == nullptr) {
        return 0;
    }
    const int rupees = static_cast<int>(Ship::OotmmSilverRupees::MaxRequired(puzzle->suffix));
    if (rupees <= 0) {
        return 0;
    }
    *base = puzzle->base;
    *width = rupees;
    return 1;
}

extern "C" void OotmmSilverRupeesSyncFlags(PlayState* play) {
    if (!ShuffleActive()) {
        return;
    }
    for (const Puzzle& puzzle : kPuzzles) {
        const bool masterQuest = OotmmSession_IsSceneMasterQuest(puzzle.scene) != 0;
        if (puzzle.flag < 0 || puzzle.masterQuest != masterQuest ||
            !(OwnsMagicalRupee() || Satisfied(puzzle, masterQuest))) {
            continue;
        }
        if (play != nullptr && play->sceneNum == puzzle.scene) {
            Flags_SetSwitch(play, puzzle.flag);
        } else {
            gSaveContext.sceneFlags[puzzle.scene].swch |= 1u << puzzle.flag;
        }
    }
}

extern "C" void OotmmSilverRupeesUpdateTempFlags(void) {
    if (gPlayState == nullptr || !ShuffleActive() || !OotmmSession_IsSceneMasterQuest(gPlayState->sceneNum)) {
        return;
    }
    static constexpr struct {
        int16_t Scene;
        const char* Suffix;
        int32_t Flag;
    } kTempPuzzles[] = {
        { SCENE_DODONGOS_CAVERN, "DC", 0x25 },
        { SCENE_SPIRIT_TEMPLE, "SPIRIT_LOBBY", 0x37 },
    };
    for (const auto& puzzle : kTempPuzzles) {
        if (gPlayState->sceneNum != puzzle.Scene) {
            continue;
        }
        const std::string suffix(puzzle.Suffix);
        const auto& inventory = OotmmIpc_GetInventory();
        const uint32_t required = Ship::OotmmSilverRupees::Required(puzzle.Suffix, true);
        if (OwnsMagicalRupee() || inventory.Has("OOT_POUCH_SILVER_" + suffix) ||
            (required != 0 && inventory.Count("OOT_RUPEE_SILVER_" + suffix) >= required)) {
            Flags_SetSwitch(gPlayState, puzzle.Flag);
        }
    }
}

extern "C" int OotmmSilverRupeesSceneBase(int scene) {
    if (!ShuffleActive()) {
        return -1;
    }
    int lowest = -1;
    for (const Puzzle& puzzle : kPuzzles) {
        if (puzzle.scene == scene && (lowest < 0 || puzzle.base < lowest)) {
            lowest = puzzle.base;
        }
    }
    return lowest;
}

extern "C" int OotmmSilverRupeesRequired(const char* suffix) {
    if (suffix == nullptr) {
        return 0;
    }
    for (const Puzzle& puzzle : kPuzzles) {
        if (std::string_view(puzzle.suffix) != suffix) {
            continue;
        }
        const bool masterQuest = OotmmSession_IsSceneMasterQuest(puzzle.scene) != 0;
        return static_cast<int>(Ship::OotmmSilverRupees::Required(suffix, masterQuest));
    }
    return 0;
}
