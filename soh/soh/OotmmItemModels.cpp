#include "OotmmItemModels.h"

#include "libultraship/bridge/OotmmItemRender.h"
#include "soh/ResourceManagerHelpers.h"

#include <cmath>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "global.h"

extern "C" {
#include "functions.h"
#include "macros.h"
#include "z64.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_gi_bomb_2/object_gi_bomb_2.h"
#include "objects/object_gi_fire/object_gi_fire.h"
#include "objects/object_gi_heart/object_gi_heart.h"
#include "objects/object_gi_magicpot/object_gi_magicpot.h"
#include "objects/object_gi_rupy/object_gi_rupy.h"
#include "objects/object_gi_sutaru/object_gi_sutaru.h"
#include "overlays/effects/ovl_Effect_Ss_Fhg_Flash/z_eff_ss_fhg_flash.h"
}

namespace {

constexpr const char* kOcarinaButtonModels[] = {
    "__OTR__objects/object_ocarina_a_button/gOcarinaAButtonDL",
    "__OTR__objects/object_ocarina_c_up_button/gOcarinaCUpButtonDL",
    "__OTR__objects/object_ocarina_c_down_button/gOcarinaCDownButtonDL",
    "__OTR__objects/object_ocarina_c_left_button/gOcarinaCLeftButtonDL",
    "__OTR__objects/object_ocarina_c_right_button/gOcarinaCRightButtonDL",
};

enum ForeignModelKind {
    FM_OPA0,
    FM_OPA01,
    FM_OPA0_XLU1,
    FM_XLU01,
    FM_OPA1023,
    FM_WALLET,
    FM_MAGIC_ARROW,
    FM_POTION,
    FM_COMPASS,
    FM_SKULL_TOKEN,
    FM_DEKU_NUTS,
    FM_SMALL_RUPEE,
    FM_OPA10_XLU32,
    FM_HEART,
    FM_OPA01_DL26,
    FM_BLUE_FIRE,
    FM_JEWEL,
    FM_FAIRY_BOTTLE,
    FM_MOONS_TEAR,
    FM_REMAINS,
    FM_BOMBCHU_DL23,
    FM_MUSIC_NOTE_ENV,
    FM_OWL_STATUE,
    FM_CLOCK,
    FM_SCALE,
    FM_OPA10_XLU2,
    FM_GORON_SWORD,
    FM_MIRROR_SHIELD,
    FM_MASTER_SWORD,
    FM_DOUBLE_DEFENSE,
};

struct ForeignModel {
    ForeignModelKind Kind;
    int32_t Param;
    const char* DisplayLists[8];
};

constexpr uint8_t kJewelColors[][4][3] = {
    { { 255, 255, 160 }, { 0, 255, 0 }, { 255, 255, 170 }, { 150, 120, 0 } },
    { { 255, 170, 255 }, { 255, 0, 100 }, { 255, 255, 170 }, { 150, 120, 0 } },
    { { 50, 255, 255 }, { 50, 0, 150 }, { 255, 255, 170 }, { 150, 120, 0 } },
};

Gfx kBronzeScaleMainColor[] = {
    gsDPPipeSync(),
    gsDPSetPrimColor(0x00, 0x80, 255, 200, 100, 255),
    gsDPSetEnvColor(150, 100, 50, 255),
    gsSPEndDisplayList(),
};

Gfx kBronzeScaleWaterColor[] = {
    gsDPPipeSync(),
    gsDPSetPrimColor(0x00, 0x60, 255, 255, 255, 255),
    gsDPSetEnvColor(255, 145, 0, 255),
    gsSPEndDisplayList(),
};

Gfx kTrapMagicJarColorSilver[] = {
    gsDPSetPrimColor(0x00, 0x00, 255, 255, 255, 255),
    gsDPSetEnvColor(50, 60, 60, 255),
    gsSPEndDisplayList(),
};

Gfx kTrapMagicJarColorBlack[] = {
    gsDPSetPrimColor(0x00, 0x00, 30, 30, 30, 255),
    gsDPSetEnvColor(10, 10, 10, 255),
    gsSPEndDisplayList(),
};

Gfx* LoadOptionalGfx(const char* path) {
    return ResourceMgr_FileExists(path) ? ResourceMgr_LoadGfxByName(path) : nullptr;
}

bool DrawOpaPaths(PlayState* play, std::initializer_list<const char*> paths) {
    std::vector<Gfx*> displayLists;
    displayLists.reserve(paths.size());
    for (const char* path : paths) {
        Gfx* displayList = LoadOptionalGfx(path);
        if (displayList == nullptr) {
            return false;
        }
        displayLists.push_back(displayList);
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetPrimColor(gfxCtx->polyOpa.p++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(gfxCtx->polyOpa.p++, 255, 255, 255, 255);
    for (Gfx* displayList : displayLists) {
        gSPDisplayList(gfxCtx->polyOpa.p++, displayList);
    }
    return true;
}

bool DrawOpaXluPaths(PlayState* play, const char* opaPath, const char* xluPath) {
    Gfx* opa = LoadOptionalGfx(opaPath);
    Gfx* xlu = LoadOptionalGfx(xluPath);
    if (opa == nullptr || xlu == nullptr) {
        return false;
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, opa);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, xlu);
    return true;
}

bool DrawOcarinaButton(PlayState* play, uint32_t variant) {
    if (variant >= std::size(kOcarinaButtonModels)) {
        return false;
    }
    Gfx* displayList = LoadOptionalGfx(kOcarinaButtonModels[variant]);
    if (displayList == nullptr) {
        return false;
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    Matrix_Push();
    Matrix_Scale(0.6f, 0.6f, 0.6f, MTXMODE_APPLY);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    Matrix_Pop();
    gSPDisplayList(gfxCtx->polyOpa.p++, displayList);
    return true;
}

bool DrawNative(PlayState* play, int32_t drawId) {
    GetItem_Draw(play, drawId);
    return true;
}

bool DrawTrap(PlayState* play, const Ship::OotmmRenderRecipe& recipe) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const int32_t frames = static_cast<int32_t>(play->state.frames);
    const int32_t variant = recipe.Param;

    if (variant == 0) {
        Gfx* model = LoadOptionalGfx(gEffIceFragment3DL);
        if (model == nullptr) {
            return false;
        }
        Gfx_SetupDL_25Xlu(gfxCtx);
        gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                   (uintptr_t)Gfx_TwoTexScroll(gfxCtx, 0, 0, (-frames) & 0x7f, 32, 32, 1, 0,
                                               (-frames * 2) & 0x7f, 32, 32));
        Matrix_Push();
        Matrix_Translate(0.0f, -25.0f, 0.0f, MTXMODE_APPLY);
        Matrix_Scale(0.5f, 0.5f, 0.5f, MTXMODE_APPLY);
        gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gDPSetEnvColor(gfxCtx->polyXlu.p++, 0, 50, 100, 255);
        gSPDisplayList(gfxCtx->polyXlu.p++, model);
        Matrix_Pop();
        return true;
    }

    if (variant == 1) {
        Gfx* model = LoadOptionalGfx(gEffFire1DL);
        if (model == nullptr) {
            return false;
        }
        Gfx_SetupDL_25Xlu(gfxCtx);
        Matrix_Push();
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Translate(0.0f, -30.0f, -15.0f, MTXMODE_APPLY);
        Matrix_Scale(0.0055f * 1.7f, 0.0055f, 0.0055f, MTXMODE_APPLY);
        gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                   (uintptr_t)Gfx_TwoTexScroll(gfxCtx, 0, 0, 0, 32, 64, 1, 0,
                                               ((-frames) & 0x7f) << 2, 32, 128));
        gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        Matrix_Pop();
        gDPSetPrimColor(gfxCtx->polyXlu.p++, 0x80, 0x80, 255, 96, 0, 192);
        gDPSetEnvColor(gfxCtx->polyXlu.p++, 255, 192, 0, 192);
        gSPDisplayList(gfxCtx->polyXlu.p++, model);
        return true;
    }

    if (variant == 2) {
        Gfx_SetupDL_25Xlu(gfxCtx);
        Matrix_Push();
        Matrix_Scale(3.0f, 3.0f, 3.0f, MTXMODE_APPLY);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_RotateZ(Rand_ZeroOne() * M_PI, MTXMODE_APPLY);
        gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        Matrix_Pop();
        gDPSetRenderMode(gfxCtx->polyXlu.p++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
        gDPPipeSync(gfxCtx->polyXlu.p++);
        gDPSetPrimColor(gfxCtx->polyXlu.p++, 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(gfxCtx->polyXlu.p++, 0, 255, 155, 0);
        gSPDisplayList(gfxCtx->polyXlu.p++, EffectSsFhgFlash_GetShockDList());
        return true;
    }

    if (variant == 3) {
        const std::string path =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 0, Ship::OotmmModelGame::Oot);
        Gfx* model = LoadOptionalGfx(path.c_str());
        if (model == nullptr) {
            return false;
        }
        Gfx_SetupDL_25Xlu(gfxCtx);
        gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                   (uintptr_t)Gfx_TwoTexScroll(gfxCtx, 0, 0, -(frames * 3), 32, 32, 1, 0,
                                               -(frames * 2), 32, 32));
        gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gDPSetCombineLERP(gfxCtx->polyXlu.p++, TEXEL1, TEXEL0, PRIM_LOD_FRAC, TEXEL0,
                          TEXEL1, TEXEL0, ENVIRONMENT, TEXEL0, ENVIRONMENT, PRIMITIVE,
                          COMBINED, PRIMITIVE, 0, 0, 0, COMBINED);
        gDPSetPrimColor(gfxCtx->polyXlu.p++, 0, 0x80, 20, 20, 20, 255);
        gDPSetEnvColor(gfxCtx->polyXlu.p++, 10, 10, 10, 255);
        gSPDisplayList(gfxCtx->polyXlu.p++, model);
        return true;
    }

    if (variant == 4) {
        const std::string path =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 0, Ship::OotmmModelGame::Oot);
        Gfx* model = LoadOptionalGfx(path.c_str());
        if (model == nullptr) {
            return false;
        }
        Gfx_SetupDL_25Opa(gfxCtx);
        gSPSegment(gfxCtx->polyOpa.p++, 0x08, (uintptr_t)kTrapMagicJarColorBlack);
        gSPSegment(gfxCtx->polyOpa.p++, 0x09, (uintptr_t)kTrapMagicJarColorSilver);
        gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gSPDisplayList(gfxCtx->polyOpa.p++, model);
        return true;
    }

    if (variant == 5) {
        const std::string path =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 0, Ship::OotmmModelGame::Oot);
        Gfx* model = LoadOptionalGfx(path.c_str());
        if (model == nullptr) {
            return false;
        }
        Gfx_SetupDL_25Opa(gfxCtx);
        Matrix_Push();
        Matrix_Scale(0.1f, 0.1f, 0.1f, MTXMODE_APPLY);
        gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gSPDisplayList(gfxCtx->polyOpa.p++, model);
        gSPSetExtraGeometryMode(gfxCtx->polyOpa.p++, G_EX_INVERT_CULLING);
        gSPDisplayList(gfxCtx->polyOpa.p++, model);
        gSPClearExtraGeometryMode(gfxCtx->polyOpa.p++, G_EX_INVERT_CULLING);
        Matrix_Pop();
        return true;
    }

    Gfx* model = LoadOptionalGfx(gGiRupeeInnerDL);
    if (variant != 6 || model == nullptr) {
        return false;
    }
    Gfx* outer = LoadOptionalGfx(gGiRupeeOuterDL);
    if (outer == nullptr) {
        return false;
    }
    Matrix_Push();
    Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetPrimColor(gfxCtx->polyOpa.p++, 0, 0x80, 20, 20, 20, 255);
    gDPSetEnvColor(gfxCtx->polyOpa.p++, 0, 0, 0, 255);
    gSPDisplayList(gfxCtx->polyOpa.p++, model);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetPrimColor(gfxCtx->polyXlu.p++, 0, 0x80, 20, 20, 20, 255);
    gDPSetEnvColor(gfxCtx->polyXlu.p++, 30, 30, 30, 255);
    gSPDisplayList(gfxCtx->polyXlu.p++, outer);
    Matrix_Pop();
    return true;
}

bool DrawForeignModel(PlayState* play, const ForeignModel& model) {
    Gfx* dl[8] = {};
    for (size_t i = 0; i < std::size(dl); i++) {
        const char* path = model.DisplayLists[i];
        if (path != nullptr && path[0] != '\0') {
            dl[i] = LoadOptionalGfx(path);
            if (dl[i] == nullptr) {
                return false;
            }
        }
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const int32_t frames = static_cast<int32_t>(play->state.frames);
    const auto loadOpaMatrix = [&]() {
        gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
    };
    const auto loadXluMatrix = [&]() {
        gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
    };

    switch (model.Kind) {
        case FM_OPA0:
        case FM_OPA01:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            if (model.Kind == FM_OPA01) {
                gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            }
            break;
        case FM_OPA0_XLU1:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        case FM_XLU01:
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[0]);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        case FM_OPA1023:
        case FM_WALLET:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[2]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[3]);
            if (model.Kind == FM_WALLET) {
                for (size_t i = 4; i < std::size(dl) && dl[i] != nullptr; i++) {
                    gSPDisplayList(gfxCtx->polyOpa.p++, dl[i]);
                }
            }
            break;
        case FM_MAGIC_ARROW:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[2]);
            break;
        case FM_SCALE: {
            Gfx* mainColor = model.Param == 1 ? kBronzeScaleMainColor
                             : model.Param == 3 ? dl[5]
                                              : dl[2];
            Gfx* waterColor = model.Param == 1 ? kBronzeScaleWaterColor
                              : model.Param == 3 ? dl[4]
                                               : dl[1];
            Gfx_SetupDL_25Xlu(gfxCtx);
            gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, frames * 2, -(frames * 2), 64, 64,
                                                     1, frames * 4, -(frames * 4), 32, 32, 2, -2,
                                                     4, -4));
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, mainColor);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[3]);
            gSPDisplayList(gfxCtx->polyXlu.p++, waterColor);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[0]);
            break;
        }
        case FM_POTION:
            Gfx_SetupDL_25Opa(gfxCtx);
            gSPSegment(gfxCtx->polyOpa.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, -frames, frames, 32, 32, 1, -frames,
                                                     frames, 32, 32, -1, 1, -1, 1));
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[2]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[3]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[4]);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[5]);
            break;
        case FM_COMPASS:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            gfxCtx->polyXlu.p = Gfx_SetupDL(gfxCtx->polyXlu.p, 5);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        case FM_SKULL_TOKEN:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, -(frames * 5), 32, 32, 1, 0, 0,
                                                     32, 64, 0, -5, 0, 0));
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        case FM_DEKU_NUTS:
            Gfx_SetupDL_25Opa(gfxCtx);
            gSPSegment(gfxCtx->polyOpa.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, frames * 6, frames * 6, 32, 32, 1,
                                                     frames * 6, frames * 6, 32, 32, 6, 6, 6, 6));
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            break;
        case FM_SMALL_RUPEE:
        case FM_OPA10_XLU32:
            if (model.Kind == FM_SMALL_RUPEE) {
                Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);
            }
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[3]);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[2]);
            break;
        case FM_HEART:
            Gfx_SetupDL_25Xlu(gfxCtx);
            gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, -(frames * 3), 32, 32, 1, 0,
                                                     -(frames * 2), 32, 32, 0, -3, 0, -2));
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[0]);
            break;
        case FM_OPA01_DL26:
            Gfx_SetupDL_26Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            break;
        case FM_BLUE_FIRE:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, 0, 16, 32, 1, frames,
                                                     -(frames * 8), 16, 32, 0, 0, 1, -8));
            Matrix_Push();
            Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            Matrix_Pop();
            break;
        case FM_JEWEL: {
            const size_t variant =
                model.Param >= 0 && model.Param < static_cast<int32_t>(std::size(kJewelColors))
                    ? static_cast<size_t>(model.Param)
                    : 0;
            const auto& color = kJewelColors[variant];
            gSPSegment(gfxCtx->polyXlu.p++, 9,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, 255, 64, 64, 1, 0, 255,
                                                     16, 16, 0, 0, 0, 0));
            gSPSegment(gfxCtx->polyOpa.p++, 8,
                       (uintptr_t)Gfx_TexScrollEx(gfxCtx, 0, 0, 16, 16, 0, 0));
            Matrix_Push();
            Matrix_RotateZYX(0, -0x4000, 0x4000, MTXMODE_APPLY);
            loadXluMatrix();
            loadOpaMatrix();
            Gfx_SetupDL_25Xlu(gfxCtx);
            gDPSetPrimColor(gfxCtx->polyXlu.p++, 0, 128, color[0][0], color[0][1],
                            color[0][2], 255);
            gDPSetEnvColor(gfxCtx->polyXlu.p++, color[1][0], color[1][1], color[1][2], 255);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[0]);
            Gfx_SetupDL_25Opa(gfxCtx);
            gDPSetPrimColor(gfxCtx->polyOpa.p++, 0, 128, color[2][0], color[2][1],
                            color[2][2], 255);
            gDPSetEnvColor(gfxCtx->polyOpa.p++, color[3][0], color[3][1], color[3][2], 255);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            Matrix_Pop();
            break;
        }
        case FM_FAIRY_BOTTLE:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            gSPSegment(gfxCtx->polyXlu.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, 0, 32, 32, 1, frames, -(frames * 6),
                                                     32, 320, 0, 0, 1, -6));
            Matrix_Push();
            Matrix_ReplaceRotation(&play->billboardMtxF);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[2]);
            Matrix_Pop();
            break;
        case FM_MOONS_TEAR: {
            Gfx_SetupDL_25Opa(gfxCtx);
            Gfx_SetupDL_25Xlu(gfxCtx);
            Gfx* segment9 =
                Gfx_TwoTexScrollEx(gfxCtx, 0, frames, -frames, 32, 32, 1, 0, 0, 32, 32, 1, -1, 0, 0);
            Gfx* segment10 =
                Gfx_TwoTexScrollEx(gfxCtx, 0, 0, 0, 32, 32, 1, frames, frames * 2, 32, 32, 0, 0, 1, 2);
            gSPSegment(gfxCtx->polyOpa.p++, 0x09, (uintptr_t)segment9);
            gSPSegment(gfxCtx->polyXlu.p++, 0x09, (uintptr_t)segment9);
            gSPSegment(gfxCtx->polyOpa.p++, 0x0a, (uintptr_t)segment10);
            gSPSegment(gfxCtx->polyXlu.p++, 0x0a, (uintptr_t)segment10);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        }
        case FM_REMAINS:
            Matrix_Scale(0.02f, 0.02f, 0.02f, MTXMODE_APPLY);
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            break;
        case FM_OPA10_XLU2:
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[2]);
            break;
        case FM_GORON_SWORD:
            Gfx_SetupDL_25Opa(gfxCtx);
            gSPSegment(gfxCtx->polyOpa.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, frames, 0, 32, 32, 1, 0, 0, 32, 32,
                                                     1, 0, 0, 0));
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            break;
        case FM_MIRROR_SHIELD:
            Gfx_SetupDL_25Opa(gfxCtx);
            gSPSegment(gfxCtx->polyOpa.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, (frames * 2) % 256, 64, 64, 1, 0,
                                                     frames % 128, 32, 32, 0, 2, 0, 1));
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        case FM_MASTER_SWORD:
            Gfx_SetupDL_25Opa(gfxCtx);
            gSPSegment(gfxCtx->polyOpa.p++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, frames, 0, 32, 32, 1, 0, 0, 32, 32,
                                                     1, 0, 0, 0));
            Matrix_Scale(0.05f, 0.05f, 0.05f, MTXMODE_APPLY);
            Matrix_RotateZ(2.1f, MTXMODE_APPLY);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            break;
        case FM_DOUBLE_DEFENSE:
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gDPSetGrayscaleColor(gfxCtx->polyXlu.p++, 255, 255, 255, 255);
            gSPGrayscale(gfxCtx->polyXlu.p++, true);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[0]);
            gSPGrayscale(gfxCtx->polyXlu.p++, false);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[1]);
            break;
        case FM_BOMBCHU_DL23:
            gfxCtx->polyOpa.p = Gfx_SetupDL(gfxCtx->polyOpa.p, 23);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            break;
        case FM_MUSIC_NOTE_ENV:
            Gfx_SetupDL_25Xlu(gfxCtx);
            loadXluMatrix();
            gDPSetEnvColor(gfxCtx->polyXlu.p++, (model.Param >> 16) & 0xff,
                           (model.Param >> 8) & 0xff, model.Param & 0xff, 255);
            gSPDisplayList(gfxCtx->polyXlu.p++, dl[0]);
            break;
        case FM_OWL_STATUE:
            Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
            Matrix_Translate(0.0f, -3000.0f, 0.0f, MTXMODE_APPLY);
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            break;
        case FM_CLOCK: {
            const s16 faceRotation = gSaveContext.nightFlag ? 0 : 0xc000;
            const s16 panelRotation = gSaveContext.nightFlag ? 0x8000 : 0;
            Matrix_Scale(0.015f, 0.015f, 0.015f, MTXMODE_APPLY);
            Gfx_SetupDL_25Opa(gfxCtx);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
            Matrix_RotateZ((-faceRotation * 2) * (M_PI / 32768.0f), MTXMODE_APPLY);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[2]);
            Matrix_Translate(0.0f, -1112.0f, -19.6f, MTXMODE_APPLY);
            Matrix_RotateY(panelRotation * (M_PI / 32768.0f), MTXMODE_APPLY);
            loadOpaMatrix();
            gSPDisplayList(gfxCtx->polyOpa.p++, dl[3]);
            break;
        }
    }
    return true;
}

s32 HideStrayFairyHead(PlayState*, s32 limbIndex, Gfx** displayList, Vec3f*, Vec3s*, void*, Gfx**) {
    if (limbIndex == 9) {
        *displayList = nullptr;
    }
    return false;
}

bool DrawTranscendentFairy(PlayState* play, const Ship::OotmmRenderRecipe& recipe) {
    static SkelAnime skelAnime{};
    static Vec3s jointTable[10]{};
    static Vec3s morphTable[10]{};
    static bool initialized = false;
    static std::string skeleton;
    static std::string animation;
    if (!initialized) {
        skeleton =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 0, Ship::OotmmModelGame::Oot);
        animation =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 1, Ship::OotmmModelGame::Oot);
    }
    if (!ResourceMgr_FileExists(skeleton.c_str()) || !ResourceMgr_FileExists(animation.c_str())) {
        return false;
    }
    if (!initialized) {
        SkelAnime_InitFlex(play, &skelAnime, reinterpret_cast<FlexSkeletonHeader*>(const_cast<char*>(skeleton.c_str())),
                           reinterpret_cast<AnimationHeader*>(const_cast<char*>(animation.c_str())),
                           jointTable, morphTable, std::size(jointTable));
        skelAnime.playSpeed = 1.0f;
        initialized = true;
    }
    SkelAnime_Update(&skelAnime);

    static constexpr uint8_t kFairyColors[][2][3] = {
        { { 0xd2, 0xb8, 0xc8 }, { 0xba, 0x50, 0x84 } },
        { { 0xf0, 0xf6, 0xc2 }, { 0x45, 0x85, 0x2b } },
        { { 0xe1, 0xeb, 0xfd }, { 0x7f, 0x65, 0xcc } },
        { { 0xfe, 0xfe, 0xe7 }, { 0xc2, 0xc1, 0x64 } },
        { { 0xf1, 0xe5, 0xd9 }, { 0xbc, 0x70, 0x2d } },
    };
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const size_t variant = recipe.Param >= 1 && recipe.Param <= 6
                               ? static_cast<size_t>(recipe.Param - 1)
                               : 5;
    Gfx* foreground = static_cast<Gfx*>(Graph_Alloc(gfxCtx, sizeof(Gfx) * 3));
    Gfx* background = static_cast<Gfx*>(Graph_Alloc(gfxCtx, sizeof(Gfx) * 3));
    if (foreground == nullptr || background == nullptr) {
        return false;
    }
    if (variant < std::size(kFairyColors)) {
        const auto& colors = kFairyColors[variant];
        gDPSetPrimColor(&foreground[0], 0, 0, colors[0][0], colors[0][1], colors[0][2], 255);
        gDPSetEnvColor(&foreground[1], colors[1][0], colors[1][1], colors[1][2], 255);
        gDPSetPrimColor(&background[0], 0, 0, colors[1][0], colors[1][1], colors[1][2], 255);
        gDPSetEnvColor(&background[1], colors[1][0], colors[1][1], colors[1][2], 255);
    } else {
        const float phase = static_cast<float>(play->state.frames) * 0.08f;
        const uint8_t red = static_cast<uint8_t>(128.0f + 127.0f * std::sin(phase));
        const uint8_t green = static_cast<uint8_t>(128.0f + 127.0f * std::sin(phase + 2.094f));
        const uint8_t blue = static_cast<uint8_t>(128.0f + 127.0f * std::sin(phase + 4.189f));
        gDPSetPrimColor(&foreground[0], 0, 0, 255, 255, 255, 255);
        gDPSetEnvColor(&foreground[1], 204, 204, 204, 255);
        gDPSetPrimColor(&background[0], 0, 0, red, green, blue, 255);
        gDPSetEnvColor(&background[1], red / 2, green / 2, blue / 2, 255);
    }
    gSPEndDisplayList(&foreground[2]);
    gSPEndDisplayList(&background[2]);

    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPSegment(gfxCtx->polyXlu.p++, 0x08, reinterpret_cast<uintptr_t>(foreground));
    gSPSegment(gfxCtx->polyXlu.p++, 0x09, reinterpret_cast<uintptr_t>(background));
    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(0.03f, 0.03f, 0.03f, MTXMODE_APPLY);
    gfxCtx->polyXlu.p = SkelAnime_DrawFlex(play, skelAnime.skeleton, skelAnime.jointTable,
                                           skelAnime.dListCount, HideStrayFairyHead, nullptr, nullptr,
                                           gfxCtx->polyXlu.p);
    Matrix_Pop();
    return true;
}

bool DrawPondFish(PlayState* play, std::string_view id,
                  const Ship::OotmmRenderRecipe& recipe);

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(error : 4062)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#endif
std::optional<ForeignModelKind> LocalRenderKind(Ship::OotmmRenderKind kind) {
    switch (kind) {
        case Ship::OotmmRenderKind::OPA0: return FM_OPA0;
        case Ship::OotmmRenderKind::OPA01: return FM_OPA01;
        case Ship::OotmmRenderKind::OPA0_XLU1: return FM_OPA0_XLU1;
        case Ship::OotmmRenderKind::XLU01: return FM_XLU01;
        case Ship::OotmmRenderKind::OPA1023: return FM_OPA1023;
        case Ship::OotmmRenderKind::WALLET: return FM_WALLET;
        case Ship::OotmmRenderKind::MAGIC_ARROW: return FM_MAGIC_ARROW;
        case Ship::OotmmRenderKind::POTION: return FM_POTION;
        case Ship::OotmmRenderKind::COMPASS: return FM_COMPASS;
        case Ship::OotmmRenderKind::SKULL_TOKEN: return FM_SKULL_TOKEN;
        case Ship::OotmmRenderKind::DEKU_NUTS: return FM_DEKU_NUTS;
        case Ship::OotmmRenderKind::SMALL_RUPEE: return FM_SMALL_RUPEE;
        case Ship::OotmmRenderKind::OPA10_XLU32: return FM_OPA10_XLU32;
        case Ship::OotmmRenderKind::HEART: return FM_HEART;
        case Ship::OotmmRenderKind::OPA01_DL26: return FM_OPA01_DL26;
        case Ship::OotmmRenderKind::BLUE_FIRE: return FM_BLUE_FIRE;
        case Ship::OotmmRenderKind::JEWEL: return FM_JEWEL;
        case Ship::OotmmRenderKind::FAIRY_BOTTLE:
        case Ship::OotmmRenderKind::FAIRY_CONTAINER:
            return FM_FAIRY_BOTTLE;
        case Ship::OotmmRenderKind::MOONS_TEAR: return FM_MOONS_TEAR;
        case Ship::OotmmRenderKind::REMAINS: return FM_REMAINS;
        case Ship::OotmmRenderKind::BOMBCHU_DL23:
        case Ship::OotmmRenderKind::OPA0_DL26:
            return FM_BOMBCHU_DL23;
        case Ship::OotmmRenderKind::MUSIC_NOTE:
        case Ship::OotmmRenderKind::MUSIC_NOTE_ENV:
            return FM_MUSIC_NOTE_ENV;
        case Ship::OotmmRenderKind::OWL_STATUE: return FM_OWL_STATUE;
        case Ship::OotmmRenderKind::CLOCK: return FM_CLOCK;
        case Ship::OotmmRenderKind::SCALE: return FM_SCALE;
        case Ship::OotmmRenderKind::OPA10_XLU2: return FM_OPA10_XLU2;
        case Ship::OotmmRenderKind::GORON_SWORD: return FM_GORON_SWORD;
        case Ship::OotmmRenderKind::MIRROR_SHIELD: return FM_MIRROR_SHIELD;
        case Ship::OotmmRenderKind::MASTER_SWORD: return FM_MASTER_SWORD;
        case Ship::OotmmRenderKind::DOUBLE_DEFENSE: return FM_DOUBLE_DEFENSE;
        case Ship::OotmmRenderKind::BOMBCHU_BAG:
        case Ship::OotmmRenderKind::BULLET_BAG:
        case Ship::OotmmRenderKind::KEY_RING:
        case Ship::OotmmRenderKind::POND_FISH:
        case Ship::OotmmRenderKind::RUSTY_KEY:
        case Ship::OotmmRenderKind::SKELETON_KEY:
        case Ship::OotmmRenderKind::STRAY_FAIRY:
        case Ship::OotmmRenderKind::TRAP:
        case Ship::OotmmRenderKind::COIN:
        case Ship::OotmmRenderKind::FISH:
        case Ship::OotmmRenderKind::MAGIC_SPELL:
        case Ship::OotmmRenderKind::POE:
        case Ship::OotmmRenderKind::TRIFORCE:
            return std::nullopt;
    }
    return std::nullopt;
}
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

bool DrawCatalogRecipe(PlayState* play, const std::string& itemId, const std::string& itemName) {
    const Ship::OotmmRenderRecipe* recipe = Ship::OotmmItemRenderCatalog::Find(itemId, itemName);
    if (recipe == nullptr) {
        return false;
    }
    if (recipe->Kind == Ship::OotmmRenderKind::STRAY_FAIRY) {
        return DrawTranscendentFairy(play, *recipe);
    }
    if (recipe->Kind == Ship::OotmmRenderKind::POND_FISH) {
        return DrawPondFish(play, itemId, *recipe);
    }
    if (recipe->Kind == Ship::OotmmRenderKind::TRAP) {
        return DrawTrap(play, *recipe);
    }
    std::array<std::string, 8> paths;
    for (size_t i = 0; i < paths.size(); i++) {
        paths[i] = Ship::OotmmItemRenderCatalog::ResourcePath(*recipe, i, Ship::OotmmModelGame::Oot);
    }
    if (recipe->Kind == Ship::OotmmRenderKind::BOMBCHU_BAG) {
        ForeignModel bag{ FM_OPA1023, 0, {} };
        for (size_t i = 0; i < 4; i++) {
            bag.DisplayLists[i] = paths[i].c_str();
        }
        Gfx* bombchu = LoadOptionalGfx(paths[4].c_str());
        if (bombchu == nullptr || !DrawForeignModel(play, bag)) {
            return false;
        }
        GraphicsContext* gfxCtx = play->state.gfxCtx;
        Gfx_SetupDL_26Opa(gfxCtx);
        Matrix_Push();
        Matrix_Translate(0.0f, 22.0f, 5.0f, MTXMODE_APPLY);
        Matrix_Scale(0.55f, 0.55f, 0.55f, MTXMODE_APPLY);
        gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        Matrix_Pop();
        gSPDisplayList(gfxCtx->polyOpa.p++, bombchu);
        return true;
    }
    if (recipe->Kind == Ship::OotmmRenderKind::BULLET_BAG) {
        std::array<Gfx*, 5> dl = {};
        for (size_t i = 0; i < dl.size(); i++) {
            dl[i] = LoadOptionalGfx(paths[i].c_str());
            if (dl[i] == nullptr) {
                return false;
            }
        }
        GraphicsContext* gfxCtx = play->state.gfxCtx;
        Gfx_SetupDL_25Opa(gfxCtx);
        gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
        gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
        Gfx_SetupDL_25Xlu(gfxCtx);
        gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gSPDisplayList(gfxCtx->polyXlu.p++, dl[2]);
        gSPDisplayList(gfxCtx->polyXlu.p++, dl[3]);
        gSPDisplayList(gfxCtx->polyXlu.p++, dl[4]);
        return true;
    }
    if (recipe->Kind == Ship::OotmmRenderKind::KEY_RING) {
        return DrawOpaPaths(play, { paths[0].c_str(), paths[1].c_str(), paths[2].c_str() });
    }
    if (recipe->Kind == Ship::OotmmRenderKind::RUSTY_KEY ||
        recipe->Kind == Ship::OotmmRenderKind::SKELETON_KEY) {
        Gfx* displayList = LoadOptionalGfx(paths[0].c_str());
        if (displayList == nullptr) {
            return false;
        }
        GraphicsContext* gfxCtx = play->state.gfxCtx;
        Gfx_SetupDL_25Opa(gfxCtx);
        gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD);
        gDPSetPrimColor(gfxCtx->polyOpa.p++, 0, 0, 255, 255, 255, 255);
        if (recipe->Kind == Ship::OotmmRenderKind::RUSTY_KEY) {
            gDPSetEnvColor(gfxCtx->polyOpa.p++, 140, 55, 15, 255);
        } else {
            gDPSetEnvColor(gfxCtx->polyOpa.p++, (recipe->Param >> 16) & 0xff,
                           (recipe->Param >> 8) & 0xff, recipe->Param & 0xff, 255);
        }
        gSPDisplayList(gfxCtx->polyOpa.p++, displayList);
        return true;
    }
    const auto localKind = LocalRenderKind(recipe->Kind);
    if (!localKind.has_value()) {
        return false;
    }
    ForeignModel model{
        *localKind, Ship::OotmmItemRenderCatalog::ResolveParam(*recipe, itemName), {}
    };
    for (size_t i = 0; i < paths.size(); i++) {
        model.DisplayLists[i] = paths[i].c_str();
    }
    return DrawForeignModel(play, model);
}

bool DrawForeignPotion(PlayState* play, uint32_t variant) {
    const char* potColor = variant == 1 ? "Green" : variant == 2 ? "Blue" : "Red";
    const std::string root = "__OTR__objects/mm_obj_gi_liquid/gGiPotionContainer";
    const std::string paths[] = {
        root + "PotDL", root + potColor + "PotColorDL", root + potColor + "LiquidColorDL",
        root + "LiquidDL", root + potColor + "PatternColorDL", root + "PatternDL",
    };
    Gfx* dl[6] = {};
    for (size_t i = 0; i < std::size(dl); i++) {
        dl[i] = LoadOptionalGfx(paths[i].c_str());
        if (dl[i] == nullptr) {
            return false;
        }
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const int32_t frames = static_cast<int32_t>(play->state.frames);
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPSegment(gfxCtx->polyOpa.p++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, -frames, frames, 32, 32, 1, -frames, frames,
                                             32, 32, -1, 1, -1, 1));
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, dl[1]);
    gSPDisplayList(gfxCtx->polyOpa.p++, dl[0]);
    gSPDisplayList(gfxCtx->polyOpa.p++, dl[2]);
    gSPDisplayList(gfxCtx->polyOpa.p++, dl[3]);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, dl[4]);
    gSPDisplayList(gfxCtx->polyXlu.p++, dl[5]);
    return true;
}

bool DrawForeignRupee(PlayState* play, uint32_t variant) {
    static constexpr const char* kColors[] = { "Green", "Blue", "Red", "Purple", "Silver", "Gold" };
    if (variant >= std::size(kColors)) {
        return false;
    }
    const std::string root = "__OTR__objects/mm_obj_gi_rupy/gGi";
    const std::string innerColor = root + kColors[variant] + "RupeeInnerColorDL";
    const std::string outerColor = root + kColors[variant] + "RupeeOuterColorDL";
    Gfx* inner = LoadOptionalGfx((root + "RupeeInnerDL").c_str());
    Gfx* innerTint = LoadOptionalGfx(innerColor.c_str());
    Gfx* outer = LoadOptionalGfx((root + "RupeeOuterDL").c_str());
    Gfx* outerTint = LoadOptionalGfx(outerColor.c_str());
    if (inner == nullptr || innerTint == nullptr || outer == nullptr || outerTint == nullptr) {
        return false;
    }

    if (variant <= 2) {
        Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);
    }
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, innerTint);
    gSPDisplayList(gfxCtx->polyOpa.p++, inner);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, outerTint);
    gSPDisplayList(gfxCtx->polyXlu.p++, outer);
    return true;
}

bool DrawForeignCompass(PlayState* play) {
    Gfx* compass = LoadOptionalGfx("__OTR__objects/mm_obj_gi_compass/gGiCompassDL");
    Gfx* glass = LoadOptionalGfx("__OTR__objects/mm_obj_gi_compass/gGiCompassGlassDL");
    if (compass == nullptr || glass == nullptr) {
        return false;
    }
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, compass);
    gfxCtx->polyXlu.p = Gfx_SetupDL(gfxCtx->polyXlu.p, 5);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, glass);
    return true;
}

bool DrawForeignBombchu(PlayState* play) {
    Gfx* bombchu = LoadOptionalGfx("__OTR__objects/mm_obj_gi_bomb_2/gGiBombchuDL");
    if (bombchu == nullptr) {
        return false;
    }
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    gfxCtx->polyOpa.p = Gfx_SetupDL(gfxCtx->polyOpa.p, 23);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, bombchu);
    return true;
}

bool DrawForeignFish(PlayState* play) {
    Gfx* fish = LoadOptionalGfx("__OTR__objects/mm_obj_gi_fish/gGiFishContainerDL");
    if (fish == nullptr) {
        return false;
    }
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const int32_t frames = static_cast<int32_t>(play->state.frames);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPSegment(gfxCtx->polyXlu.p++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, frames, 32, 32, 1, 0, frames, 32, 32,
                                             0, 1, 0, 1));
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, fish);
    return true;
}

bool DrawForeignPoe(PlayState* play) {
    Gfx* lid = LoadOptionalGfx("__OTR__objects/mm_obj_gi_ghost/gGiPoeContainerLidDL");
    Gfx* glass = LoadOptionalGfx("__OTR__objects/mm_obj_gi_ghost/gGiPoeContainerGlassDL");
    Gfx* contents = LoadOptionalGfx("__OTR__objects/mm_obj_gi_ghost/gGiPoeContainerContentsDL");
    Gfx* color = LoadOptionalGfx("__OTR__objects/mm_obj_gi_ghost/gGiPoeContainerPoeColorDL");
    if (lid == nullptr || glass == nullptr || contents == nullptr || color == nullptr) {
        return false;
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const int32_t frames = static_cast<int32_t>(play->state.frames);
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, lid);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, glass);
    gSPSegment(gfxCtx->polyXlu.p++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, 0, 16, 32, 1, frames, -(frames * 6), 16,
                                             32, 0, 0, 1, -6));
    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, color);
    gSPDisplayList(gfxCtx->polyXlu.p++, contents);
    Matrix_Pop();
    return true;
}

float PondFishScale(std::string_view id) {
    int pounds = 0;
    bool foundWeight = false;
    for (char c : id) {
        if (c >= '0' && c <= '9') {
            pounds = pounds * 10 + (c - '0');
            foundWeight = true;
        } else if (foundWeight) {
            break;
        }
    }
    if (pounds == 0) {
        return 1.0f;
    }

    constexpr float kWeightToLength = 0.2f;
    constexpr float kLengthOffset = 2.0f;
    constexpr float kCurveOffset = 0.5f;
    constexpr float kCurveScale = 0.0036f;
    const float base = std::sqrt((kLengthOffset - kCurveOffset) / kCurveScale) + 1.0f;
    const float actual =
        std::sqrt(((pounds * kWeightToLength + kLengthOffset) - kCurveOffset) / kCurveScale) + 1.0f;
    return actual / base;
}

bool DrawPondFish(PlayState* play, std::string_view id,
                  const Ship::OotmmRenderRecipe& recipe) {
    struct AnimatedFish {
        SkelAnime AnimationState{};
        std::string Skeleton;
        std::string Animation;
        bool Initialized = false;
    };
    static AnimatedFish fishModels[2];
    const size_t variant = recipe.Param == 1 ? 1 : 0;
    AnimatedFish& fish = fishModels[variant];
    if (!fish.Initialized) {
        fish.Skeleton =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 0, Ship::OotmmModelGame::Oot);
        fish.Animation =
            Ship::OotmmItemRenderCatalog::ResourcePath(recipe, 1, Ship::OotmmModelGame::Oot);
        if (!ResourceMgr_FileExists(fish.Skeleton.c_str()) ||
            !ResourceMgr_FileExists(fish.Animation.c_str())) {
            return false;
        }
        SkelAnime_InitFlex(
            play, &fish.AnimationState,
            reinterpret_cast<FlexSkeletonHeader*>(const_cast<char*>(fish.Skeleton.c_str())),
            reinterpret_cast<AnimationHeader*>(const_cast<char*>(fish.Animation.c_str())),
            nullptr, nullptr, 0);
        Animation_PlayLoop(
            &fish.AnimationState,
            reinterpret_cast<AnimationHeader*>(const_cast<char*>(fish.Animation.c_str())));
        fish.AnimationState.playSpeed = 1.0f;
        fish.Initialized = true;
    }
    SkelAnime_Update(&fish.AnimationState);

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    Matrix_Push();
    const float modelScale = variant == 1 ? 0.0032f : 0.006f;
    const float scale = modelScale * PondFishScale(id);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    Matrix_RotateY(-M_PI / 2.0f, MTXMODE_APPLY);
    gfxCtx->polyOpa.p =
        SkelAnime_DrawFlex(play, fish.AnimationState.skeleton, fish.AnimationState.jointTable,
                           fish.AnimationState.dListCount, nullptr, nullptr, nullptr,
                           gfxCtx->polyOpa.p);
    Matrix_Pop();
    return true;
}

bool DrawOwlStatue(PlayState* play, bool foreign) {
    const char* path = foreign ? "__OTR__objects/mm_obj_sek/gOwlStatueOpenedDL"
                               : "__OTR__objects/object_sek/gOwlStatueOpenedDL";
    Gfx* owl = LoadOptionalGfx(path);
    if (owl == nullptr) {
        return false;
    }
    Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
    Matrix_Translate(0.0f, -3000.0f, 0.0f, MTXMODE_APPLY);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, owl);
    return true;
}

bool DrawSilverRupee(PlayState* play) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Matrix_Push();
    Matrix_Scale(0.04f, 0.04f, 0.04f, MTXMODE_APPLY);
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPSegment(gfxCtx->polyOpa.p++, 0x08, (uintptr_t)SEGMENTED_TO_VIRTUAL(gRupeeSilverTex));
    gSPDisplayList(gfxCtx->polyOpa.p++, (Gfx*)gRupeeDL);
    Matrix_Pop();
    return true;
}

bool DrawSoul(PlayState* play, std::string_view id) {
    constexpr uint8_t kEnemy[] = { 0xff, 0x00, 0x00 };
    constexpr uint8_t kBoss[] = { 0x88, 0x00, 0xff };
    constexpr uint8_t kNpc[] = { 0x00, 0xff, 0x00 };
    constexpr uint8_t kAnimal[] = { 0x22, 0x22, 0xff };
    constexpr uint8_t kMisc[] = { 0x88, 0x88, 0x88 };
    const uint8_t* tint = id.find("_ENEMY_") != std::string_view::npos   ? kEnemy
                          : id.find("_BOSS_") != std::string_view::npos ? kBoss
                          : id.find("_NPC_") != std::string_view::npos  ? kNpc
                          : id.find("_ANIMAL_") != std::string_view::npos ? kAnimal
                                                                         : kMisc;
    const int32_t frames = static_cast<int32_t>(play->state.frames);
    GraphicsContext* gfxCtx = play->state.gfxCtx;

    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetGrayscaleColor(gfxCtx->polyOpa.p++, 0xd5, 0xd5, 0xd5, 255);
    gSPGrayscale(gfxCtx->polyOpa.p++, true);
    gSPDisplayList(gfxCtx->polyOpa.p++, (Gfx*)gGiSkulltulaTokenDL);
    gSPGrayscale(gfxCtx->polyOpa.p++, false);

    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPSegment(gfxCtx->polyXlu.p++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0, -(frames * 5), 32, 32, 1, 0, 0, 32, 64, 0,
                                             -5, 0, 0));
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetGrayscaleColor(gfxCtx->polyXlu.p++, tint[0], tint[1], tint[2], 255);
    gSPGrayscale(gfxCtx->polyXlu.p++, true);
    gSPDisplayList(gfxCtx->polyXlu.p++, (Gfx*)gGiSkulltulaTokenFlameDL);
    gSPGrayscale(gfxCtx->polyXlu.p++, false);
    if (Gfx* skull = LoadOptionalGfx("__OTR__objects/object_gi_sutaru/gGiSkulltulaTokenSkullDL")) {
        gDPPipeSync(gfxCtx->polyXlu.p++);
        gDPSetGrayscaleColor(gfxCtx->polyXlu.p++, 0xd5, 0xd5, 0xd5, 255);
        gSPGrayscale(gfxCtx->polyXlu.p++, true);
        gSPDisplayList(gfxCtx->polyXlu.p++, skull);
        gSPGrayscale(gfxCtx->polyXlu.p++, false);
    }
    return true;
}

void HueToRgb(float hue, uint8_t& r, uint8_t& g, uint8_t& b) {
    const float scaled = hue * 6.0f;
    const int sector = static_cast<int>(scaled) % 6;
    const uint8_t ramp = static_cast<uint8_t>((scaled - static_cast<int>(scaled)) * 255.0f);
    const uint8_t fade = static_cast<uint8_t>(255 - ramp);
    switch (sector) {
        case 0: r = 255; g = ramp; b = 0; return;
        case 1: r = fade; g = 255; b = 0; return;
        case 2: r = 0; g = 255; b = ramp; return;
        case 3: r = 0; g = fade; b = 255; return;
        case 4: r = ramp; g = 0; b = 255; return;
        default: r = 255; g = 0; b = fade; return;
    }
}

bool DrawMagicalRupee(PlayState* play) {
    Gfx* inner = LoadOptionalGfx(dgGiRupeeInnerDL);
    Gfx* outer = LoadOptionalGfx(dgGiRupeeOuterDL);
    if (inner == nullptr || outer == nullptr) {
        return false;
    }

    uint8_t r;
    uint8_t g;
    uint8_t b;
    HueToRgb(static_cast<float>(play->state.frames % 60) / 60.0f, r, g, b);

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetPrimColor(gfxCtx->polyOpa.p++, 0, 0x80, r, g, b, 255);
    gDPSetEnvColor(gfxCtx->polyOpa.p++, r / 5, g / 5, b / 5, 255);
    gSPDisplayList(gfxCtx->polyOpa.p++, inner);

    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetPrimColor(gfxCtx->polyXlu.p++, 0, 0x80, 255, 255, 255, 255);
    gDPSetEnvColor(gfxCtx->polyXlu.p++, r * 3 / 4, g * 3 / 4, b * 3 / 4, 255);
    gSPDisplayList(gfxCtx->polyXlu.p++, outer);
    return true;
}

bool DrawTintedKey(PlayState* play, std::string_view id) {
    const bool rusty = id.find("_RUSTY_KEY_") != std::string_view::npos;
    const char* path = rusty ? "__OTR__objects/object_key/gSmallKeyCustomDL"
                             : "__OTR__objects/object_gi_key/gGiSmallKeyDL";
    Gfx* displayList = LoadOptionalGfx(path);
    if (displayList == nullptr) {
        return false;
    }
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gDPSetPrimColor(gfxCtx->polyOpa.p++, 0, 0, 255, rusty ? 245 : 255, rusty ? 235 : 255, 255);
    gDPSetEnvColor(gfxCtx->polyOpa.p++, rusty ? 140 : 60, rusty ? 55 : 80, rusty ? 15 : 90, 255);
    gSPDisplayList(gfxCtx->polyOpa.p++, displayList);
    return true;
}

const char* KeyRingTheme(std::string_view id) {
    if (id.ends_with("_FIRE") || id.ends_with("_SH")) return "FireTemple";
    if (id.ends_with("_WATER") || id.ends_with("_GB")) return "WaterTemple";
    if (id.ends_with("_SPIRIT")) return "SpiritTemple";
    if (id.ends_with("_SHADOW") || id.ends_with("_ST")) return "ShadowTemple";
    if (id.ends_with("_BOTW")) return "BottomoftheWell";
    if (id.ends_with("_GTG")) return "GerudoTrainingGround";
    if (id.ends_with("_GF")) return "GerudoFortress";
    if (id.ends_with("_GANON")) return "GanonsCastle";
    if (id.ends_with("_TCG")) return "TreasureChestGame";
    return "ForestTemple";
}

bool DrawKeyRing(PlayState* play, std::string_view id) {
    const std::string theme = KeyRingTheme(id);
    const std::string root = "__OTR__objects/object_keyring/gKeyring";
    const std::string icon = root + "Icon" + theme + "DL";
    const std::string keys = root + "Keys" + theme + "DL";
    return DrawOpaPaths(play, { "__OTR__objects/object_keyring/gKeyringRingDL", keys.c_str(), icon.c_str() });
}

bool DrawSkeletonKey(PlayState* play) {
    return DrawOpaPaths(play, { "__OTR__objects/object_key/gSkeletonKeyDL" });
}

bool DrawCoin(PlayState* play, uint32_t variant) {
    static constexpr const char* kColors[] = {
        "__OTR__objects/object_gi_coin/gGiYellowCoinColorDL",
        "__OTR__objects/object_gi_coin/gGiRedCoinColorDL",
        "__OTR__objects/object_gi_coin/gGiGreenCoinColorDL",
        "__OTR__objects/object_gi_coin/gGiBlueCoinColorDL",
    };
    if (variant >= std::size(kColors)) {
        return false;
    }
    Gfx* coin = LoadOptionalGfx("__OTR__objects/object_gi_coin/gGiCoinDL");
    Gfx* color = LoadOptionalGfx(kColors[variant]);
    Gfx* emblem = LoadOptionalGfx("__OTR__objects/object_gi_coin/gGiNDL");
    if (coin == nullptr || color == nullptr || emblem == nullptr) {
        return false;
    }
    Matrix_Scale(2.0f, 2.0f, 2.0f, MTXMODE_APPLY);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    Gfx_SetupDL_25Opa(gfxCtx);
    gSPMatrix(gfxCtx->polyOpa.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyOpa.p++, color);
    gSPDisplayList(gfxCtx->polyOpa.p++, coin);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gSPMatrix(gfxCtx->polyXlu.p++, Matrix_NewMtx(gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD);
    gSPDisplayList(gfxCtx->polyXlu.p++, emblem);
    return true;
}

bool DrawSpinUpgrade(PlayState* play, std::string_view id) {
    if (id.starts_with("MM_") &&
        DrawOpaPaths(play, { "__OTR__objects/mm_obj_gi_sword_1/gGiKokiriSwordBladeHiltDL",
                             "__OTR__objects/mm_obj_gi_sword_1/gGiKokiriSwordGuardDL" })) {
        return true;
    }
    return DrawNative(play, GID_SWORD_KOKIRI);
}

bool DrawWorldMap(PlayState* play, std::string_view id) {
    if (!id.starts_with("MM_WORLD_MAP_")) {
        return false;
    }
    if (DrawOpaPaths(play, { "__OTR__objects/mm_obj_gi_fieldmap/gGiTingleMapDL",
                              "__OTR__objects/mm_obj_gi_fieldmap/gGiTingleMapEmptyDL" })) {
        return true;
    }
    return DrawNative(play, GID_DUNGEON_MAP);
}

bool EndsWith(std::string_view value, std::string_view suffix) {
    return value.ends_with(suffix);
}

int32_t EquipmentDrawId(std::string_view id) {
    if (EndsWith(id, "_HAMMER")) return GID_HAMMER;
    if (EndsWith(id, "_SLINGSHOT")) return GID_SLINGSHOT;
    if (EndsWith(id, "_BOOMERANG")) return GID_BOOMERANG;
    if (EndsWith(id, "_BOOTS_IRON")) return GID_BOOTS_IRON;
    if (EndsWith(id, "_BOOTS_HOVER")) return GID_BOOTS_HOVER;
    if (EndsWith(id, "_TUNIC_GORON")) return GID_TUNIC_GORON;
    if (EndsWith(id, "_TUNIC_ZORA")) return GID_TUNIC_ZORA;
    if (EndsWith(id, "_STONE_OF_AGONY")) return GID_STONE_OF_AGONY;
    if (EndsWith(id, "_SHIELD_DEKU")) return GID_SHIELD_DEKU;
    if (EndsWith(id, "_SHIELD_HYLIAN")) return GID_SHIELD_HYLIAN;
    if (EndsWith(id, "_SPELL_FIRE")) return GID_DINS_FIRE;
    if (EndsWith(id, "_SPELL_WIND")) return GID_FARORES_WIND;
    if (EndsWith(id, "_SPELL_LOVE")) return GID_NAYRUS_LOVE;
    if (EndsWith(id, "_BOMBCHU_BAG")) return GID_BOMBCHU;
    if (EndsWith(id, "_GREAT_FAIRY_SWORD")) return GID_SWORD_BGS;
    if (EndsWith(id, "_POWDER_KEG")) return GID_BOMB;
    if (EndsWith(id, "_SWORD_RAZOR") || EndsWith(id, "_SWORD_GILDED")) return GID_SWORD_BGS;
    if (EndsWith(id, "_MASK_BLAST")) return GID_BOMB;
    if (EndsWith(id, "_MASK_STONE")) return GID_STONE_OF_AGONY;
    if (EndsWith(id, "_MASK_KAMARO")) return GID_MASK_SPOOKY;
    return -1;
}

} // namespace

bool OotmmItemModel_Draw(PlayState* play, const Ship::OotmmItemDefinition& item) {
    if (play == nullptr) {
        return false;
    }
    switch (item.ModelKind) {
        case Ship::OotmmItemModelKind::OcarinaButton:
            return DrawOcarinaButton(play, item.ModelVariant);
        case Ship::OotmmItemModelKind::Soul:
            return DrawSoul(play, item.Id);
        case Ship::OotmmItemModelKind::PlatinumToken:
            return DrawNative(play, GID_SKULL_TOKEN);
        case Ship::OotmmItemModelKind::Key:
            return DrawTintedKey(play, item.Id);
        case Ship::OotmmItemModelKind::Song:
        case Ship::OotmmItemModelKind::SongNote:
            return DrawNative(play, GID_SONG_GENERIC);
        case Ship::OotmmItemModelKind::SilverRupee:
            return DrawSilverRupee(play);
        case Ship::OotmmItemModelKind::Coin:
            return DrawCoin(play, item.ModelVariant);
        case Ship::OotmmItemModelKind::MagicalRupee:
            return DrawMagicalRupee(play);
        case Ship::OotmmItemModelKind::Triforce:
            return DrawNative(play, GID_TRIFORCE_PIECE);
        case Ship::OotmmItemModelKind::Clock:
            return DrawNative(play, GID_SONG_TIME);
        case Ship::OotmmItemModelKind::Equipment: {
            const int32_t drawId = EquipmentDrawId(item.Id);
            return drawId >= 0 && DrawNative(play, drawId);
        }
        case Ship::OotmmItemModelKind::Upgrade:
            if (item.Id.ends_with("_SPIN_UPGRADE")) {
                return DrawSpinUpgrade(play, item.Id);
            }
            return DrawNative(play, item.Id.find("SCALE") != std::string::npos ? GID_SCALE_SILVER
                                                                               : GID_BRACELET);
        case Ship::OotmmItemModelKind::OwlStatue:
            return DrawOwlStatue(play, item.Id.starts_with("MM_"));
        case Ship::OotmmItemModelKind::Fishing:
            if (const auto* recipe = Ship::OotmmItemRenderCatalog::Find(item.Id);
                recipe != nullptr && recipe->Kind == Ship::OotmmRenderKind::POND_FISH) {
                return DrawPondFish(play, item.Id, *recipe);
            }
            return false;
        case Ship::OotmmItemModelKind::Fairy:
            return DrawNative(play, GID_FAIRY);
        case Ship::OotmmItemModelKind::Potion:
            if (item.Id.starts_with("MM_")) {
                return DrawForeignPotion(play, item.ModelVariant);
            }
            return DrawNative(play, item.ModelVariant == 1 ? GID_POTION_GREEN
                                    : item.ModelVariant == 2 ? GID_POTION_BLUE
                                                             : GID_POTION_RED);
        case Ship::OotmmItemModelKind::Rupee:
            if (item.Id.starts_with("MM_")) {
                return DrawForeignRupee(play, item.ModelVariant);
            }
            return DrawNative(play, item.ModelVariant == 1 ? GID_RUPEE_BLUE
                                    : item.ModelVariant == 2 ? GID_RUPEE_RED
                                    : item.ModelVariant == 3 ? GID_RUPEE_PURPLE
                                                             : GID_RUPEE_GOLD);
        case Ship::OotmmItemModelKind::Bombchu:
            return item.Id.starts_with("MM_") ? DrawForeignBombchu(play) : DrawNative(play, GID_BOMBCHU);
        case Ship::OotmmItemModelKind::Compass:
            return item.Id.starts_with("MM_") ? DrawForeignCompass(play) : DrawNative(play, GID_COMPASS);
        case Ship::OotmmItemModelKind::DungeonMap:
            if (item.Id.starts_with("MM_WORLD_MAP_")) {
                return DrawWorldMap(play, item.Id);
            }
            if (item.Id.starts_with("MM_")) {
                return DrawOpaPaths(play, { "__OTR__objects/mm_obj_gi_map/gGiDungeonMapDL" });
            }
            return DrawNative(play, GID_DUNGEON_MAP);
        case Ship::OotmmItemModelKind::Fish:
            return item.Id.starts_with("MM_") ? DrawForeignFish(play) : DrawNative(play, GID_FISH);
        case Ship::OotmmItemModelKind::KeyRing:
            return DrawKeyRing(play, item.Id);
        case Ship::OotmmItemModelKind::SkeletonKey:
            return DrawSkeletonKey(play);
        case Ship::OotmmItemModelKind::MagicJar:
            return DrawNative(play, item.ModelVariant == 1 ? GID_MAGIC_LARGE : GID_MAGIC_SMALL);
        case Ship::OotmmItemModelKind::Poe:
            return item.Id.starts_with("MM_") ? DrawForeignPoe(play) : DrawNative(play, GID_POE);
        case Ship::OotmmItemModelKind::Trap:
            if (item.ModelVariant == 3) return DrawNative(play, GID_HEART_CONTAINER);
            if (item.ModelVariant == 4) return DrawNative(play, GID_MAGIC_SMALL);
            if (item.ModelVariant == 5) return DrawNative(play, GID_BOMBCHU);
            if (item.ModelVariant == 6) return DrawNative(play, GID_RUPEE_PURPLE);
            return DrawNative(play, GID_BLUE_FIRE);
        default:
            return false;
    }
}

bool OotmmItemModel_DrawById(PlayState* play, const std::string& itemId, const std::string& itemName) {
    const auto item = Ship::OotmmItemCatalog::DescribeModel(itemId, itemName);
    const bool customFirst = itemId.find("_SOUL_") != std::string::npos ||
                             itemId.find("_COIN_") != std::string::npos ||
                             itemId.find("_RUPEE_SILVER_") != std::string::npos ||
                             itemId.find("_POUCH_SILVER_") != std::string::npos;
    if (customFirst && item.has_value() && OotmmItemModel_Draw(play, *item)) {
        return true;
    }
    if (!customFirst && DrawCatalogRecipe(play, itemId, itemName)) {
        return true;
    }
    if (!customFirst && item.has_value() && OotmmItemModel_Draw(play, *item)) {
        return true;
    }
    if (DrawWorldMap(play, itemId)) {
        return true;
    }
    const int32_t drawId = EquipmentDrawId(itemId);
    return drawId >= 0 && DrawNative(play, drawId);
}
