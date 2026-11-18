#include "OotmmCsmc.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"
#include "soh/frame_interpolation.h"

#include <libultraship/bridge/OotmmCsmc.h>

#include <string>
#include <unordered_map>

extern "C" {
#include "functions.h"
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

std::unordered_map<EnBox*, Ship::OotmmCsmcClass> sChestOverrides;

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

constexpr f32 kSmallChestScale = 0.005f;
constexpr f32 kLargeChestScale = 0.01f;

void SetVanillaLook(EnBox* chest) {
    switch (chest->type) {
        case ENBOX_TYPE_SMALL:
        case ENBOX_TYPE_6:
        case ENBOX_TYPE_ROOM_CLEAR_SMALL:
        case ENBOX_TYPE_SWITCH_FLAG_FALL_SMALL:
            Actor_SetScale(&chest->dyna.actor, kSmallChestScale);
            Actor_SetFocus(&chest->dyna.actor, 20.0f);
            break;
        default:
            Actor_SetScale(&chest->dyna.actor, kLargeChestScale);
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

void ApplyClassLook(EnBox* chest, Ship::OotmmCsmcClass itemClass) {
    const bool large = itemClass == Ship::OotmmCsmcClass::Major ||
                       itemClass == Ship::OotmmCsmcClass::BossKey;
    Actor_SetScale(&chest->dyna.actor, large ? kLargeChestScale : kSmallChestScale);
    Actor_SetFocus(&chest->dyna.actor, large ? 40.0f : 20.0f);

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
}

bool Revealing() {
    const auto mode = Ship::OotmmCsmc_Mode(OotmmSession_GetState());
    return mode == Ship::OotmmCsmcMode::Always || (mode == Ship::OotmmCsmcMode::Agony && HasAgonyStone());
}

Ship::OotmmCsmcClass RevealedClass(const Ship::OotmmPlacement& placement) {
    return Ship::OotmmCsmc_Classify(OotmmSession_GetState(), Ship::OotmmGame::Oot, placement, true);
}

void* LoadCsmcDL(const char* family, const char* category) {
    const std::string path = std::string("__OTR__objects/ootmm_csmc/gCsmc") + family + category + "DL";
    return ResourceMgr_LoadGfxByName(path.c_str());
}

const char* CategoryName(Ship::OotmmCsmcClass itemClass) {
    switch (itemClass) {
        case Ship::OotmmCsmcClass::BossKey:
            return "BossKey";
        case Ship::OotmmCsmcClass::Major:
            return "Major";
        case Ship::OotmmCsmcClass::Key:
            return "Key";
        case Ship::OotmmCsmcClass::Spider:
            return "Spider";
        case Ship::OotmmCsmcClass::Fairy:
            return "Fairy";
        case Ship::OotmmCsmcClass::Heart:
            return "Heart";
        case Ship::OotmmCsmcClass::Soul:
            return "Soul";
        case Ship::OotmmCsmcClass::MapCompass:
            return "Map";
        default:
            return nullptr;
    }
}

} // namespace

void* OotmmCsmc_ContainerDL(const Ship::OotmmPlacement& placement, const char* family) {
    const char* category = Revealing() ? CategoryName(RevealedClass(placement)) : "Major";
    return category != nullptr ? LoadCsmcDL(family, category) : nullptr;
}

uint32_t OotmmCsmc_TreeColor(const Ship::OotmmPlacement* placement) {
    if (placement == nullptr) {
        return 0x32AA46;
    }
    switch (Revealing() ? RevealedClass(*placement) : Ship::OotmmCsmcClass::Major) {
        case Ship::OotmmCsmcClass::BossKey:
            return 0x0000FF;
        case Ship::OotmmCsmcClass::Major:
            return 0xFFFF00;
        case Ship::OotmmCsmcClass::Key:
            return 0x444444;
        case Ship::OotmmCsmcClass::Spider:
            return 0xFFFFFF;
        case Ship::OotmmCsmcClass::Fairy:
            return 0xFF7AFB;
        case Ship::OotmmCsmcClass::Heart:
            return 0xFF0000;
        case Ship::OotmmCsmcClass::Soul:
            return 0x340B9C;
        case Ship::OotmmCsmcClass::MapCompass:
            return 0xC75000;
        default:
            return 0x32AA46;
    }
}

void* OotmmCsmc_SkulltulaDL(const Ship::OotmmPlacement* placement, const char* part) {
    if (placement == nullptr || !Revealing() || !OotmmSession_GetState().GetBoolSetting("csmcSkulltula", false)) {
        return nullptr;
    }
    const auto itemClass = RevealedClass(*placement);
    if (itemClass == Ship::OotmmCsmcClass::Major) {
        return nullptr;
    }
    const char* category = itemClass == Ship::OotmmCsmcClass::Normal ? "Normal" : CategoryName(itemClass);
    return category != nullptr ? LoadCsmcDL(part, category) : nullptr;
}

void* OotmmCsmc_CowRingDL(const Ship::OotmmPlacement* placement, bool collected, void* vanilla) {
    if (placement == nullptr) {
        return nullptr;
    }
    const bool renewable = Ship::OotmmCsmc_IsRenewable(placement->ItemId);
    if (collected && !renewable) {
        return nullptr;
    }
    if (!Revealing() || !OotmmSession_GetState().GetBoolSetting("csmcCow", false)) {
        return vanilla;
    }
    const auto itemClass = RevealedClass(*placement);
    const char* category =
        itemClass == Ship::OotmmCsmcClass::Normal ? (renewable ? "Renewable" : "Normal") : CategoryName(itemClass);
    void* dl = category != nullptr ? LoadCsmcDL("CowRing", category) : nullptr;
    return dl != nullptr ? dl : vanilla;
}

int32_t OotmmCsmc_UpdateChest(EnBox* chest, PlayState* play) {
    if (const auto override = sChestOverrides.find(chest); override != sChestOverrides.end()) {
        ApplyClassLook(chest, override->second);
        return 1;
    }
    if (!OotmmSession_IsActive()) {
        return 0;
    }

    const auto& state = OotmmSession_GetState();
    const auto* placement = state.FindCheck(Ship::OotmmGame::Oot, play->sceneNum, "chest",
                                            chest->dyna.actor.params & 0x1F);
    if (placement == nullptr) {
        return 0;
    }

    const bool excluded =
        play->sceneNum == SCENE_TREASURE_BOX_SHOP && (chest->dyna.actor.params & 0x1F) != 0x0A &&
        state.GetStringSetting("smallKeyShuffleChestGame", "vanilla") == "vanilla";
    const auto mode = Ship::OotmmCsmc_Mode(state);
    if (mode == Ship::OotmmCsmcMode::Never || excluded) {
        SetVanillaLook(chest);
        return 1;
    }

    const bool revealed = mode == Ship::OotmmCsmcMode::Always || HasAgonyStone();
    ApplyClassLook(chest, Ship::OotmmCsmc_Classify(state, Ship::OotmmGame::Oot, *placement, revealed));
    return 1;
}

bool OotmmCsmc_IsLargeChest(const EnBox* chest) {
    return chest->dyna.actor.scale.x > kSmallChestScale;
}

void OotmmCsmc_SetChestOverride(EnBox* chest, Ship::OotmmCsmcClass itemClass) {
    sChestOverrides[chest] = itemClass;
}

void OotmmCsmc_ClearChestOverrides() {
    sChestOverrides.clear();
}

void OotmmCsmc_DrawGlitter(PlayState* play, Actor* actor, const Ship::OotmmPlacement& placement, float height) {
    static Vtx sVtx[] = {
        VTX(-10, -10, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF),
        VTX(10, -10, 0, 0x400, 0, 0xFF, 0xFF, 0xFF, 0xFF),
        VTX(10, 10, 0, 0x400, 0x400, 0xFF, 0xFF, 0xFF, 0xFF),
        VTX(-10, 10, 0, 0, 0x400, 0xFF, 0xFF, 0xFF, 0xFF),
    };
    const float distance = actor->xzDistToPlayer;
    if (distance > 1000.0f) {
        return;
    }
    const float alpha = distance <= 300.0f ? 1.0f : (1000.0f - distance) / 700.0f;
    const float alphaNoZ = distance > 600.0f ? 0.0f : distance <= 300.0f ? 1.0f : (600.0f - distance) / 300.0f;

    const auto& state = OotmmSession_GetState();
    const auto mode = Ship::OotmmCsmc_Mode(state);
    const bool enabled = mode == Ship::OotmmCsmcMode::Always ||
                         (mode == Ship::OotmmCsmcMode::Agony && HasAgonyStone());
    const uint32_t color = Ship::OotmmCsmc_GlitterColor(
        Ship::OotmmCsmc_Classify(state, Ship::OotmmGame::Oot, placement, true), enabled);
    const u8 r = (color >> 16) & 0xFF;
    const u8 g = (color >> 8) & 0xFF;
    const u8 b = color & 0xFF;

    Matrix_Translate(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, MTXMODE_NEW);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Translate(0.0f, height, 0.0f, MTXMODE_APPLY);

    OPEN_DISPS(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPLoadTextureBlock_4b(POLY_XLU_DISP++, Ship::OotmmCsmc_GlitterFrame(play->gameplayFrames), G_IM_FMT_I, 32, 32, 0,
                           G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK,
                           G_TX_NOLOD, G_TX_NOLOD);
    gDPSetCombineLERP(POLY_XLU_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, COMBINED, 0, 0, 0,
                      COMBINED);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BACK | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, r, g, b, static_cast<u8>(alpha * 0.9f * 255));
    gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(sVtx), 4, 0);
    gSP2Triangles(POLY_XLU_DISP++, 0, 2, 1, 0, 0, 3, 2, 0);
    if (alphaNoZ > 0.0f) {
        gSPClearGeometryMode(POLY_XLU_DISP++, G_ZBUFFER);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, r, g, b, static_cast<u8>(alphaNoZ * 0.25f * 255));
        gSP2Triangles(POLY_XLU_DISP++, 0, 2, 1, 0, 0, 3, 2, 0);
        gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}
