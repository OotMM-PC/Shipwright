#include "OotmmSunlightArrows.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

extern "C" {
#include "z64.h"
#include "overlays/actors/ovl_Obj_Lightswitch/z_obj_lightswitch.h"
}

void OotmmSunlightArrows_Init() {
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorInit>(ACTOR_OBJ_LIGHTSWITCH, [](void* actor) {
        if (OotmmSession_BoolSetting("sunlightArrows")) {
            static_cast<ObjLightswitch*>(actor)->colliderItems[0].info.bumper.dmgFlags |= DMG_ARROW_LIGHT;
        }
    });
}
