#include "OotmmSmallKeyDoors.h"

#include "OotmmDungeons.h"
#include "OotmmSession.h"

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64.h"
}

extern "C" int OotmmSmallKeyDoorIsOpen(PlayState* play, Actor* actor) {
    if (!OotmmSession_IsActive() || play == nullptr || actor == nullptr) {
        return 0;
    }
    return OotmmDungeons_DoorIsForcedOpen(play->sceneNum, actor->params & 0x3F) ||
                   OotmmDungeons_SmallKeysRemoved(play->sceneNum)
               ? 1
               : 0;
}
