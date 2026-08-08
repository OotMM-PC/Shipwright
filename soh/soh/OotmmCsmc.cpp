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

Gfx* LoadChestDL(const char* name, const char* fallback, const char* last = nullptr) {
    Gfx* dl = name != nullptr ? ResourceMgr_LoadGfxByName(name) : nullptr;
    if (dl == nullptr && fallback != nullptr) {
        dl = ResourceMgr_LoadGfxByName(fallback);
    }
    if (dl == nullptr && last != nullptr) {
        dl = ResourceMgr_LoadGfxByName(last);
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

    // The launcher bakes OoTMM's own chest art into ootmm_assets.o2r; SoH's shipped
    // variants cover a build where that archive is not mounted yet.
    const char* bakedBody = nullptr;
    const char* bakedLid = nullptr;
    const char* body;
    const char* lid;
    switch (itemClass) {
        case Ship::OotmmCsmcClass::BossKey:
            body = gTreasureChestBossKeyChestFrontDL;
            lid = gTreasureChestBossKeyChestSideAndTopDL;
            break;
        case Ship::OotmmCsmcClass::Major:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestMajorBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestMajorLidDL";
            body = gChestBodyMajorDL;
            lid = gChestLidMajorDL;
            break;
        case Ship::OotmmCsmcClass::Soul:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestSoulBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestSoulLidDL";
            body = gChestBodyMajorDL;
            lid = gChestLidMajorDL;
            break;
        case Ship::OotmmCsmcClass::Key:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestKeyBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestKeyLidDL";
            body = gChestBodySmallKeyDL;
            lid = gChestLidSmallKeyDL;
            break;
        case Ship::OotmmCsmcClass::Spider:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestSpiderBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestSpiderLidDL";
            body = gChestBodyTokenDL;
            lid = gChestLidTokenDL;
            break;
        case Ship::OotmmCsmcClass::Fairy:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestFairyBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestFairyLidDL";
            body = "__OTR__objects/object_box/gChestBodyFairyDL";
            lid = "__OTR__objects/object_box/gChestLidFairyDL";
            break;
        case Ship::OotmmCsmcClass::Heart:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestHeartBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestHeartLidDL";
            body = gChestBodyHeartDL;
            lid = gChestLidHeartDL;
            break;
        case Ship::OotmmCsmcClass::MapCompass:
            bakedBody = "__OTR__objects/ootmm_csmc/gCsmcChestMapBodyDL";
            bakedLid = "__OTR__objects/ootmm_csmc/gCsmcChestMapLidDL";
            body = gChestBodyMinorDL;
            lid = gChestLidMinorDL;
            break;
        default:
            body = gTreasureChestChestFrontDL;
            lid = gTreasureChestChestSideAndLidDL;
            break;
    }
    chest->boxBodyDL = LoadChestDL(bakedBody, body, gTreasureChestChestFrontDL);
    chest->boxLidDL = LoadChestDL(bakedLid, lid, gTreasureChestChestSideAndLidDL);
    return 1;
}
