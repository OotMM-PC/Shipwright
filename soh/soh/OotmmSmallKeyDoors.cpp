#include "OotmmSmallKeyDoors.h"

#include "OotmmDungeons.h"
#include "OotmmIpc.h"
#include "OotmmSession.h"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64.h"
}

namespace {

bool SkeletonKeyCovers(int sceneNum) {
    if (OotmmDungeons_MaxSmallKeys(sceneNum) == 0) {
        return false;
    }
    const auto& state = OotmmSession_GetState();
    if (!state.GetBoolSetting("skeletonKeyOot", false)) {
        return false;
    }
    return OotmmIpc_GetInventory().Has(
        state.GetBoolSetting("sharedSkeletonKey", false) ? "SHARED_SKELETON_KEY" : "OOT_SKELETON_KEY");
}

} // namespace

extern "C" int OotmmSmallKeyDoorIsOpen(PlayState* play, Actor* actor) {
    if (!OotmmSession_IsActive() || play == nullptr || actor == nullptr) {
        return 0;
    }
    if (OotmmDungeons_DoorIsForcedOpen(play->sceneNum, actor->params & 0x3F) ||
        OotmmDungeons_SmallKeysRemoved(play->sceneNum)) {
        return 1;
    }
    return SkeletonKeyCovers(play->sceneNum) ? 1 : 0;
}
