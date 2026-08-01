#include <array>

#include "global.h"

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

#include "OotmmSoaringMap.h"
}

#include "soh/ResourceManagerHelpers.h"

namespace {

constexpr const char* kWorldMapTex = "__OTR__mm_icon_item_field_static/gWorldMapImageTex";
constexpr const char* kWorldMapTlut = "__OTR__mm_icon_item_field_static/gWorldMapImageTLUT";
constexpr const char* kOwlFaceTex = "__OTR__mm_icon_item_field_static/gWorldMapOwlFaceTex";
constexpr const char* kCursorTex = "__OTR__mm_icon_item_static_yar/gPauseMenuCursorTex";

constexpr std::array<const char*, 15> kPageTextures = { {
    "__OTR__mm_icon_item_static_yar/gPauseMap00Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap01Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap02Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap03Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap04Tex",
    "__OTR__mm_icon_item_jpn_static/gPauseMap10ENGTex",
    "__OTR__mm_icon_item_static_yar/gPauseMap11Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap12Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap13Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap14Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap20Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap21Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap22Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap23Tex",
    "__OTR__mm_icon_item_static_yar/gPauseMap24Tex",
} };

constexpr std::array<const char*, 10> kNameTextures = { {
    "__OTR__mm_map_name_static/gMapPointGreatBayCoastENGTex",
    "__OTR__mm_map_name_static/gMapPointZoraCapeENGTex",
    "__OTR__mm_map_name_static/gMapPointSnowheadENGTex",
    "__OTR__mm_map_name_static/gMapPointMountainVillageENGTex",
    "__OTR__mm_map_name_static/gMapPointClockTownENGTex",
    "__OTR__mm_map_name_static/gMapPointMilkRoadENGTex",
    "__OTR__mm_map_name_static/gMapPointWoodfallENGTex",
    "__OTR__mm_map_name_static/gMapPointSouthernSwampENGTex",
    "__OTR__mm_map_name_static/gMapPointIkanaCanyonENGTex",
    "__OTR__mm_map_name_static/gMapPointStoneTowerENGTex",
} };

constexpr std::array<s16, 10> kOwlQuadX = { { -80, -64, -9, -3, -7, -16, -1, 23, 44, 54 } };
constexpr std::array<s16, 10> kOwlQuadY = { { -8, -38, 39, 26, 1, -7, -28, -27, -1, 24 } };

constexpr std::array<f32, 10> kOwlCursorX = { { -50.0f, -38.0f, 6.0f, 11.0f, 8.0f, 0.0f, 12.0f, 31.0f, 48.0f, 56.0f } };
constexpr std::array<f32, 10> kOwlCursorY = { { -14.0f, -39.0f, 23.0f, 11.0f, -8.0f, -15.0f, -31.0f, -30.0f, -10.0f,
                                                11.0f } };

// Kaleido vertices project through a 60 degree view whose eye sits PAUSE_EYE_DIST from the origin,
// so a page's screen scale is fixed by the depth its matrix translates to.
constexpr f32 kProjection = 207.846f;
constexpr f32 kEyeDistance = 64.0f;
constexpr f32 kPageScale = 0.78f * kProjection / (kEyeDistance + 93.0f);
constexpr f32 kCursorScale = kProjection / (kEyeDistance + 100.0f);

constexpr f32 kCenterX = 160.0f;
constexpr f32 kCenterY = 120.0f;
// The world map's matrix drops it 0.9 units, and lands it at a depth that projects one to one.
constexpr f32 kMapCenterY = 120.9f;

constexpr s16 kMapWidth = 216;
constexpr s16 kMapHeight = 128;
constexpr s16 kMapStrip = 8;
constexpr f32 kMapQuadX = -109.0f;
constexpr f32 kMapQuadY = 59.0f;
constexpr s16 kOwlWidth = 24;
constexpr s16 kOwlHeight = 12;
constexpr s16 kNameWidth = 128;
constexpr s16 kNameHeight = 16;
constexpr s16 kCursorSize = 16;
constexpr f32 kCursorRadius = 15.0f;

constexpr s16 kBackdropAlpha = 140;
constexpr s16 kOwlWarpAlpha = 120;

s16 sCursorSpinPhase = 0;
f32 sCursorShrinkRate = 0.0f;

bool Exists(const char* path) {
    return ResourceMgr_FileExists(path) != 0;
}

void TexRect(Gfx** gfxp, f32 left, f32 top, s16 width, s16 height, f32 scale) {
    Gfx* gfx = *gfxp;
    const s32 step = (s32)(1024.0f / scale);

    gSPTextureRectangle(gfx++, (s32)(left * 4.0f), (s32)(top * 4.0f), (s32)((left + width * scale) * 4.0f),
                        (s32)((top + height * scale) * 4.0f), G_TX_RENDERTILE, 0, 0, step, step);

    *gfxp = gfx;
}

void DrawBackdrop(Gfx** gfxp) {
    Gfx* gfx = *gfxp;

    gDPPipeSync(gfx++);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, kBackdropAlpha);
    // The renderer widens exactly this rectangle to cover the pillarbox, as it does for screen fades.
    gDPFillRectangle(gfx++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);

    *gfxp = gfx;
}

void DrawPageBackground(Gfx** gfxp) {
    Gfx* gfx = *gfxp;

    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, 180, 180, 120, 255);

    for (s32 column = 0; column < 3; column++) {
        for (s32 row = 0; row < 5; row++) {
            const f32 left = kCenterX + (-120.0f + column * 80.0f) * kPageScale;
            const f32 top = kCenterY - (80.0f - row * 32.0f) * kPageScale;

            gDPPipeSync(gfx++);
            gDPLoadTextureBlock(gfx++, kPageTextures[column * 5 + row], G_IM_FMT_IA, G_IM_SIZ_8b, 80, 32, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
            TexRect(&gfx, left, top, 80, 32, kPageScale);
        }
    }

    *gfxp = gfx;
}

void DrawWorldMap(Gfx** gfxp) {
    Gfx* gfx = *gfxp;

    gDPPipeSync(gfx++);
    gDPSetTextureFilter(gfx++, G_TF_POINT);
    gDPSetCombineMode(gfx++, G_CC_MODULATEI_PRIM, G_CC_MODULATEI_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, 255);
    gDPLoadTLUT_pal256(gfx++, kWorldMapTlut);
    gDPSetTextureLUT(gfx++, G_TT_RGBA16);

    for (s16 strip = 0; strip < kMapHeight / kMapStrip; strip++) {
        gDPLoadMultiTile(gfx++, kWorldMapTex, 0, G_TX_RENDERTILE, G_IM_FMT_CI, G_IM_SIZ_8b, kMapWidth, kMapHeight, 0,
                         strip * kMapStrip, kMapWidth - 1, (strip + 1) * kMapStrip - 1, 0,
                         G_TX_WRAP | G_TX_NOMIRROR, G_TX_WRAP | G_TX_NOMIRROR, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                         G_TX_NOLOD);
        gDPSetTileSize(gfx++, G_TX_RENDERTILE, 0, 0, (kMapWidth - 1) << G_TEXTURE_IMAGE_FRAC,
                       (kMapStrip - 1) << G_TEXTURE_IMAGE_FRAC);
        TexRect(&gfx, kCenterX + kMapQuadX, kMapCenterY - kMapQuadY + strip * kMapStrip, kMapWidth, kMapStrip, 1.0f);
    }

    gDPPipeSync(gfx++);
    gDPSetTextureLUT(gfx++, G_TT_NONE);
    gDPSetTextureFilter(gfx++, G_TF_BILERP);
    gDPSetRenderMode(gfx++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gfx++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gfx++, 0, 0, 0, 0, 0, kOwlWarpAlpha);
    gDPFillRectangle(gfx++, 50, 62, 270, 190);

    *gfxp = gfx;
}

void DrawOwlPoints(Gfx** gfxp, const uint8_t* owls, int32_t count) {
    Gfx* gfx = *gfxp;

    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, 255);
    gDPLoadTextureBlock(gfx++, kOwlFaceTex, G_IM_FMT_RGBA, G_IM_SIZ_32b, kOwlWidth, kOwlHeight, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    for (int32_t i = 0; i < count; i++) {
        const uint8_t owl = owls[i];
        TexRect(&gfx, kCenterX + kOwlQuadX[owl], kMapCenterY - kOwlQuadY[owl], kOwlWidth, kOwlHeight, 1.0f);
    }

    *gfxp = gfx;
}

void DrawCursor(Gfx** gfxp, uint8_t owl) {
    Gfx* gfx = *gfxp;

    f32 radius = kCursorRadius - sCursorShrinkRate;
    if (radius < 0.0f) {
        radius = 0.0f;
    }

    const f32 centerX = kCenterX + kOwlCursorX[owl] * kCursorScale;
    const f32 centerY = kCenterY - kOwlCursorY[owl] * kCursorScale;
    const f32 half = kCursorSize / 2.0f * kCursorScale;

    gDPPipeSync(gfx++);
    gDPSetCombineLERP(gfx++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0, PRIMITIVE,
                      ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(gfx++, 0, 0, 100, 150, 255, 255);
    gDPSetEnvColor(gfx++, 0, 0, 100, 255);
    gDPLoadTextureBlock(gfx++, kCursorTex, G_IM_FMT_IA, G_IM_SIZ_8b, kCursorSize, kCursorSize, 0,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                        G_TX_NOLOD);

    for (s32 i = 0; i < 4; i++) {
        const s16 angle = (s16)(sCursorSpinPhase + i * 0x4000);
        const f32 offsetX = Math_SinS(angle) * radius * kCursorScale;
        const f32 offsetY = Math_CosS(angle) * radius * kCursorScale;

        TexRect(&gfx, centerX + offsetX - half, centerY - offsetY - half, kCursorSize, kCursorSize, kCursorScale);
    }

    gDPPipeSync(gfx++);
    gDPSetEnvColor(gfx++, 0, 0, 0, 255);

    *gfxp = gfx;
}

void DrawPointName(Gfx** gfxp, uint8_t owl) {
    Gfx* gfx = *gfxp;

    gDPPipeSync(gfx++);
    gDPSetCombineMode(gfx++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetPrimColor(gfx++, 0, 0, 255, 255, 255, 255);
    gDPLoadTextureBlock_4b(gfx++, kNameTextures[owl], G_IM_FMT_IA, kNameWidth, kNameHeight, 0,
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                           G_TX_NOLOD);
    TexRect(&gfx, kCenterX - 63.0f, kCenterY + 80.0f, kNameWidth, kNameHeight, 1.0f);

    *gfxp = gfx;
}

} // namespace

extern "C" int32_t OotmmSoaringMap_Available(void) {
    if (!Exists(kWorldMapTex) || !Exists(kWorldMapTlut) || !Exists(kOwlFaceTex) || !Exists(kCursorTex)) {
        return 0;
    }
    for (const char* path : kPageTextures) {
        if (!Exists(path)) {
            return 0;
        }
    }
    for (const char* path : kNameTextures) {
        if (!Exists(path)) {
            return 0;
        }
    }
    return 1;
}

extern "C" void OotmmSoaringMap_CursorMoved(void) {
    sCursorShrinkRate = 4.0f;
}

extern "C" void OotmmSoaringMap_Draw(PlayState* play, const uint8_t* owls, int32_t count, int32_t cursor) {
    if (play == nullptr || owls == nullptr || count <= 0 || cursor < 0 || cursor >= count) {
        return;
    }

    sCursorSpinPhase += 0x300;
    if (sCursorShrinkRate > 0.0f) {
        sCursorShrinkRate -= 1.0f;
    }

    Gfx* gfx = play->state.gfxCtx->overlay.p;

    Gfx_SetupDL_39Ptr(&gfx);
    DrawBackdrop(&gfx);
    DrawPageBackground(&gfx);
    DrawWorldMap(&gfx);
    DrawOwlPoints(&gfx, owls, count);
    DrawCursor(&gfx, owls[cursor]);
    DrawPointName(&gfx, owls[cursor]);

    play->state.gfxCtx->overlay.p = gfx;
}
