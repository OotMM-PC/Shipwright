#include "OotmmCsmc.h"

// Pre-loads the C++ half of the game headers; the extern "C" block below needs it first.
#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

#include <libultraship/bridge/OotmmCsmc.h>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "src/overlays/actors/ovl_En_Box/z_en_box.h"
#include "objects/object_box/object_box.h"
#include "soh_assets.h"

void Actor_SetScale(Actor* actor, f32 scale);
void Actor_SetFocus(Actor* actor, f32 height);
Gfx* ResourceMgr_LoadGfxByName(const char* path);
}

namespace {

// Every grant path — shared or separate, launcher or debug — lands in the quest bit.
bool HasAgonyStone() {
    return CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY);
}

Gfx* LoadChestDL(const char* name, const char* fallback) {
    Gfx* dl = ResourceMgr_LoadGfxByName(name);
    if (dl == nullptr && fallback != nullptr) {
        dl = ResourceMgr_LoadGfxByName(fallback);
    }
    return dl;
}

void SetVanillaLook(EnBox* chest) {
    switch (chest->type) {
        case ENBOX_TYPE_SMALL:
        case ENBOX_TYPE_6:
        case ENBOX_TYPE_ROOM_CLEAR_SMALL:
        case ENBOX_TYPE_SWITCH_FLAG_FALL_SMALL:
            Actor_SetScale(&chest->dyna.actor, 0.005f);
            Actor_SetFocus(&chest->dyna.actor, 20.0f);
            break;
        default:
            Actor_SetScale(&chest->dyna.actor, 0.01f);
            Actor_SetFocus(&chest->dyna.actor, 40.0f);
    }

    if (chest->type != ENBOX_TYPE_DECORATED_BIG) {
        chest->boxBodyDL = LoadChestDL(gTreasureChestChestFrontDL, nullptr);
        chest->boxLidDL = LoadChestDL(gTreasureChestChestSideAndLidDL, nullptr);
    } else {
        chest->boxBodyDL = LoadChestDL(gTreasureChestBossKeyChestFrontDL, gTreasureChestChestFrontDL);
        chest->boxLidDL =
            LoadChestDL(gTreasureChestBossKeyChestSideAndTopDL, gTreasureChestChestSideAndLidDL);
    }
}

} // namespace

int32_t OotmmCsmc_UpdateChest(EnBox* chest, PlayState* play) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }

    const auto& state = OotmmSession_GetState();
    const auto* placement = state.FindCheck(Ship::OotmmGame::Oot, play->sceneNum, "chest",
                                            chest->dyna.actor.params & 0x1F);
    if (placement == nullptr) {
        return 0;
    }

    // The treasure-game rooms keep vanilla chests unless their keys are shuffled;
    // the final room's reward chest always participates.
    const bool excluded =
        play->sceneNum == SCENE_TREASURE_BOX_SHOP && chest->dyna.actor.room != 6 &&
        state.GetStringSetting("smallKeyShuffleChestGame", "vanilla") == "vanilla";
    const auto mode = Ship::OotmmCsmc_Mode(state);
    if (mode == Ship::OotmmCsmcMode::Never || excluded) {
        SetVanillaLook(chest);
        return 1;
    }

    const bool revealed = mode == Ship::OotmmCsmcMode::Always || HasAgonyStone();
    const auto itemClass =
        Ship::OotmmCsmc_Classify(state, Ship::OotmmGame::Oot, *placement, revealed);

    const bool large = itemClass == Ship::OotmmCsmcClass::Major ||
                       itemClass == Ship::OotmmCsmcClass::BossKey;
    Actor_SetScale(&chest->dyna.actor, large ? 0.01f : 0.005f);
    Actor_SetFocus(&chest->dyna.actor, large ? 40.0f : 20.0f);

    const char* body;
    const char* lid;
    switch (itemClass) {
        case Ship::OotmmCsmcClass::BossKey:
            body = gTreasureChestBossKeyChestFrontDL;
            lid = gTreasureChestBossKeyChestSideAndTopDL;
            break;
        // Souls borrow the major look until they have art of their own.
        case Ship::OotmmCsmcClass::Major:
        case Ship::OotmmCsmcClass::Soul:
            body = gChestBodyMajorDL;
            lid = gChestLidMajorDL;
            break;
        case Ship::OotmmCsmcClass::Key:
            body = gChestBodySmallKeyDL;
            lid = gChestLidSmallKeyDL;
            break;
        case Ship::OotmmCsmcClass::Spider:
            body = gChestBodyTokenDL;
            lid = gChestLidTokenDL;
            break;
        case Ship::OotmmCsmcClass::Fairy:
            body = "__OTR__objects/object_box/gChestBodyFairyDL";
            lid = "__OTR__objects/object_box/gChestLidFairyDL";
            break;
        case Ship::OotmmCsmcClass::Heart:
            body = gChestBodyHeartDL;
            lid = gChestLidHeartDL;
            break;
        case Ship::OotmmCsmcClass::MapCompass:
            body = gChestBodyMinorDL;
            lid = gChestLidMinorDL;
            break;
        default:
            body = gTreasureChestChestFrontDL;
            lid = gTreasureChestChestSideAndLidDL;
            break;
    }
    chest->boxBodyDL = LoadChestDL(body, gTreasureChestChestFrontDL);
    chest->boxLidDL = LoadChestDL(lid, gTreasureChestChestSideAndLidDL);
    return 1;
}
