#include "OotmmSilverRupeeLocations.h"

#include "OotmmSilverRupees.h"

#include <cstdint>

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64.h"
}

namespace {

constexpr uint16_t kPuzzleKeyMask = 0xF03F;

bool ResolveBit(PlayState* play, int32_t id, uint32_t** storage, uint32_t* mask) {
    if (play == nullptr || id < 0) {
        return false;
    }
    const int32_t scene = play->sceneNum;
    if (scene < 0 || scene >= (int32_t)(sizeof(gSaveContext.sceneFlags) / sizeof(gSaveContext.sceneFlags[0]))) {
        return false;
    }
    const int32_t origin = OotmmSilverRupeesSceneBase(scene);
    if (origin < 0) {
        return false;
    }
    const int32_t bit = id - origin;
    if (bit < 0 || bit >= 32) {
        return false;
    }
    *storage = &gSaveContext.sceneFlags[scene].unk;
    *mask = 1u << bit;
    return true;
}

} // namespace

// TODO: these track collection only; each rupee should be a real check granting its shuffled item,
// which needs the check system and seed placement data.
extern "C" int32_t OotmmSilverRupeeLocations_Claim(PlayState* play, Actor* actor) {
    int base;
    int width;

    if (play == nullptr || actor == nullptr ||
        !OotmmSilverRupeesPuzzleIds(play->sceneNum, actor->room, &base, &width)) {
        return -1;
    }

    const uint16_t key = (uint16_t)((uint16_t)actor->params & kPuzzleKeyMask);
    int32_t spawned = 0;
    for (Actor* other = play->actorCtx.actorLists[ACTORCAT_PROP].head; other != nullptr; other = other->next) {
        if (other == actor || other->id != actor->id || other->room != actor->room) {
            continue;
        }
        if (((uint16_t)other->params & kPuzzleKeyMask) == key) {
            spawned++;
        }
    }
    if (spawned >= width) {
        return -1;
    }
    return base + spawned;
}

extern "C" int32_t OotmmSilverRupeeLocations_Taken(PlayState* play, int32_t id) {
    uint32_t* storage;
    uint32_t mask;

    if (!ResolveBit(play, id, &storage, &mask)) {
        return 0;
    }
    return (*storage & mask) != 0;
}

extern "C" void OotmmSilverRupeeLocations_MarkTaken(PlayState* play, int32_t id) {
    uint32_t* storage;
    uint32_t mask;

    if (ResolveBit(play, id, &storage, &mask)) {
        *storage |= mask;
    }
}

extern "C" int32_t OotmmSilverRupeeLocations_TakenCount(PlayState* play, int32_t room, int32_t total) {
    int base;
    int width;

    if (play == nullptr || !OotmmSilverRupeesPuzzleIds(play->sceneNum, room, &base, &width)) {
        return 0;
    }

    const int32_t counted = total < width ? total : width;
    int32_t taken = 0;
    for (int32_t i = 0; i < counted; i++) {
        taken += OotmmSilverRupeeLocations_Taken(play, base + i);
    }
    return taken;
}
