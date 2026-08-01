#include "OotmmItemPage.h"

#include "OotmmCustomItems.h"
#include "OotmmSession.h"
#include "ResourceManagerHelpers.h"

#include <libultraship/bridge/consolevariablebridge.h>

#include <vector>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "assets/textures/parameter_static/parameter_static.h"

extern PlayState* gPlayState;

void KaleidoScope_DrawQuadTextureRGBA32(GraphicsContext* gfxCtx, void* texture, u16 width, u16 height,
                                        u16 point);
Gfx* KaleidoScope_QuadTextureIA8(Gfx* gfx, void* texture, s16 width, s16 height, u16 point);
void KaleidoScope_SetCursorVtx(PauseContext* pauseCtx, u16 index, Vtx* vtx);
void KaleidoScope_DrawCursor(PlayState* play, u16 pageIndex);
void KaleidoScope_SetupItemEquip(PlayState* play, u16 item, u16 slot, s16 animX, s16 animY);
}

namespace {

constexpr size_t kSlotsPerPage = 24;
constexpr size_t kColumns = 6;

size_t sPage = 0;
size_t sCursor = 0;
uint8_t sHovered = ITEM_NONE;

std::vector<uint8_t> OwnedItems() {
    std::vector<uint8_t> owned;
    const int count = OotmmCustomItems_OwnedItemCount();
    owned.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; i++) {
        owned.push_back(OotmmCustomItems_OwnedItemAt(i));
    }
    return owned;
}

size_t FirstOnPage() {
    return sPage == 0 ? 0 : (sPage - 1) * kSlotsPerPage;
}

size_t CustomPageCount() {
    const size_t owned = static_cast<size_t>(OotmmCustomItems_OwnedItemCount());
    return owned == 0 ? 0 : (owned + kSlotsPerPage - 1) / kSlotsPerPage;
}

size_t TotalPages() {
    return 1 + CustomPageCount();
}

bool PageHasItems() {
    return sPage != 0 && static_cast<size_t>(OotmmCustomItems_OwnedItemCount()) > FirstOnPage();
}

Gfx* DrawAmmoDigit(PlayState* play, Gfx* gfx, const Vtx* icon, int ammo, u8 shade) {
    static const char* const kDigits[] = {
        gAmmoDigit0Tex, gAmmoDigit1Tex, gAmmoDigit2Tex, gAmmoDigit3Tex, gAmmoDigit4Tex,
        gAmmoDigit5Tex, gAmmoDigit6Tex, gAmmoDigit7Tex, gAmmoDigit8Tex, gAmmoDigit9Tex,
    };
    if (ammo < 0 || ammo > 9) {
        return gfx;
    }

    Vtx* digit = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, 4 * sizeof(Vtx)));
    if (digit == nullptr) {
        return gfx;
    }

    const s16 left = static_cast<s16>(icon[0].v.ob[0] + 18);
    const s16 top = static_cast<s16>(icon[0].v.ob[1] - 22);
    for (int i = 0; i < 4; i++) {
        digit[i] = icon[i];
        digit[i].v.ob[0] = static_cast<s16>(left + ((i & 1) ? 8 : 0));
        digit[i].v.ob[1] = static_cast<s16>(top - ((i & 2) ? 8 : 0));
        digit[i].v.tc[0] = static_cast<s16>((i & 1) ? 8 << 5 : 0);
        digit[i].v.tc[1] = static_cast<s16>((i & 2) ? 8 << 5 : 0);
    }

    const u8 tint = ammo == 0 ? 130 : shade;

    gDPPipeSync(gfx++);
    gDPSetPrimColor(gfx++, 0, 0, tint, tint, tint, play->pauseCtx.alpha);
    gSPVertex(gfx++, (uintptr_t)digit, 4, 0);
    gDPLoadTextureBlock(gfx++, kDigits[ammo], G_IM_FMT_IA, G_IM_SIZ_8b, 8, 8, 0, G_TX_NOMIRROR | G_TX_WRAP,
                        G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gSP1Quadrangle(gfx++, 0, 2, 3, 1, 0);
    return gfx;
}

} // namespace

extern "C" int OotmmItemPage_Active(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    if (!PageHasItems()) {
        sPage = 0;
        return 0;
    }
    return 1;
}

extern "C" int OotmmItemPage_ConsumePageToggle(void) {
    if (gPlayState == nullptr) {
        return 0;
    }
    PauseContext* pauseCtx = &gPlayState->pauseCtx;
    Input* input = &gPlayState->state.input[0];
    if (!OotmmSession_IsActive() || pauseCtx->pageIndex != PAUSE_ITEM) {
        sPage = 0;
        return 0;
    }

    const size_t pages = TotalPages();
    if (sPage >= pages) {
        sPage = 0;
    }

    if (pages > 1 && CHECK_BTN_ALL(input->press.button, BTN_L)) {
        sPage = (sPage + 1) % pages;
        Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        return 1;
    }

    if (sPage != 0 && (CHECK_BTN_ALL(input->press.button, BTN_Z) ||
                       CHECK_BTN_ALL(input->press.button, BTN_R))) {
        sPage = 0;
    }
    return 0;
}

extern "C" int OotmmItemPage_EquipOutlineIndex(uint8_t cButtonSlot) {
    if (!OotmmItemPage_Active()) {
        return OotmmCustomItems_IsCustomSlot(cButtonSlot) || cButtonSlot >= kSlotsPerPage
                   ? -1
                   : static_cast<int>(cButtonSlot);
    }
    if (!OotmmCustomItems_IsCustomSlot(cButtonSlot)) {
        return -1;
    }

    const uint8_t item = OotmmCustomItems_ItemInSlot(cButtonSlot);
    const std::vector<uint8_t> owned = OwnedItems();
    const size_t first = FirstOnPage();
    for (size_t slot = 0; slot < kSlotsPerPage && first + slot < owned.size(); slot++) {
        if (owned[first + slot] == item) {
            return static_cast<int>(slot);
        }
    }
    return -1;
}

extern "C" void OotmmItemPage_UpdateCursor(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    const std::vector<uint8_t> owned = OwnedItems();
    const size_t first = FirstOnPage();
    const size_t onPage = owned.size() > first ? owned.size() - first : 0;
    if (onPage == 0) {
        sHovered = ITEM_NONE;
        return;
    }
    if (sCursor >= onPage) {
        sCursor = onPage - 1;
    }

    // An empty vanilla inventory parks the cursor on a page-turn arrow, which blocks ours.
    pauseCtx->cursorSpecialPos = 0;

    if (pauseCtx->state == 6 && pauseCtx->unk_1E4 == 0) {
        const size_t before = sCursor;
        if (pauseCtx->stickRelX < -30 && sCursor % kColumns != 0) {
            sCursor--;
        } else if (pauseCtx->stickRelX > 30 && (sCursor % kColumns) != kColumns - 1 &&
                   sCursor + 1 < onPage) {
            sCursor++;
        } else if (pauseCtx->stickRelY > 30 && sCursor >= kColumns) {
            sCursor -= kColumns;
        } else if (pauseCtx->stickRelY < -30 && sCursor + kColumns < onPage) {
            sCursor += kColumns;
        }
        if (sCursor != before) {
            Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                   &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        }
    }

    sHovered = owned[first + sCursor];
    pauseCtx->cursorItem[PAUSE_ITEM] = sHovered;
    pauseCtx->cursorSlot[PAUSE_ITEM] = static_cast<s16>(sCursor);
    pauseCtx->cursorPoint[PAUSE_ITEM] = static_cast<s16>(sCursor);
    pauseCtx->cursorX[PAUSE_ITEM] = static_cast<s16>(sCursor % kColumns);
    pauseCtx->cursorY[PAUSE_ITEM] = static_cast<s16>(sCursor / kColumns);
    KaleidoScope_SetCursorVtx(pauseCtx, static_cast<u16>(sCursor * 4), pauseCtx->itemVtx);

    u16 buttons = BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT;
    if (CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0)) {
        buttons |= BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT;
    }
    Input* input = &play->state.input[0];
    if (pauseCtx->state == 6 && pauseCtx->unk_1E4 == 0 &&
        CHECK_BTN_ANY(input->press.button, buttons)) {
        const u16 index = static_cast<u16>(sCursor * 4);
        KaleidoScope_SetupItemEquip(play, sHovered, OotmmCustomItems_SlotOf(sHovered),
                                    pauseCtx->itemVtx[index].v.ob[0] * 10,
                                    pauseCtx->itemVtx[index].v.ob[1] * 10);
    }
}

extern "C" const char* OotmmItemPage_HoveredName(void) {
    return OotmmCustomItems_DisplayName(sHovered);
}

extern "C" void OotmmItemPage_Draw(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    const std::vector<uint8_t> owned = OwnedItems();
    const size_t first = FirstOnPage();

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_42Opa(play->state.gfxCtx);

    gDPSetCombineLERP(POLY_OPA_DISP++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE,
                      0, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, PRIMITIVE, 0);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 0);

    for (size_t button = 0; button < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); button++) {
        if (button >= 3 && !CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0)) {
            continue;
        }
        const int index = OotmmItemPage_EquipOutlineIndex(gSaveContext.equips.cButtonSlots[button]);
        if (index < 0) {
            continue;
        }

        Vtx* outline = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, 4 * sizeof(Vtx)));
        if (outline == nullptr) {
            break;
        }
        const Vtx* source = &pauseCtx->itemVtx[index * 4];
        for (int i = 0; i < 4; i++) {
            outline[i] = source[i];
        }
        outline[0].v.ob[0] = outline[2].v.ob[0] = source[0].v.ob[0] - 2;
        outline[1].v.ob[0] = outline[3].v.ob[0] = outline[0].v.ob[0] + 32;
        outline[0].v.ob[1] = outline[1].v.ob[1] = source[0].v.ob[1] + 2;
        outline[2].v.ob[1] = outline[3].v.ob[1] = outline[0].v.ob[1] - 32;

        gSPVertex(POLY_OPA_DISP++, (uintptr_t)outline, 4, 0);
        POLY_OPA_DISP = KaleidoScope_QuadTextureIA8(POLY_OPA_DISP, (void*)gEquippedItemOutlineTex, 32, 32, 0);
    }

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    for (size_t slot = 0; slot < kSlotsPerPage && first + slot < owned.size(); slot++) {
        const uint8_t item = owned[first + slot];
        char* texture = ResourceMgr_LoadTexOrDListByName(OotmmCustomItems_IconPath(item));
        if (texture == nullptr) {
            continue;
        }

        Vtx* vtx = &pauseCtx->itemVtx[slot * 4];
        if (slot == sCursor) {
            vtx[0].v.ob[0] = vtx[2].v.ob[0] = vtx[0].v.ob[0] - 2;
            vtx[1].v.ob[0] = vtx[3].v.ob[0] = vtx[0].v.ob[0] + 32;
            vtx[0].v.ob[1] = vtx[1].v.ob[1] = vtx[0].v.ob[1] + 2;
            vtx[2].v.ob[1] = vtx[3].v.ob[1] = vtx[0].v.ob[1] - 32;
        }

        const u8 shade = OotmmCustomItems_UsableNow(item) ? 255 : 100;
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, shade, shade, shade, pauseCtx->alpha);
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)vtx, 4, 0);
        KaleidoScope_DrawQuadTextureRGBA32(play->state.gfxCtx, texture, 32, 32, 0);
        POLY_OPA_DISP = DrawAmmoDigit(play, POLY_OPA_DISP, vtx, OotmmCustomItems_AmmoOf(item), shade);
    }

    if (sHovered != ITEM_NONE) {
        KaleidoScope_SetCursorVtx(pauseCtx, (u16)(sCursor * 4), pauseCtx->itemVtx);
        KaleidoScope_DrawCursor(play, PAUSE_ITEM);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void OotmmItemPage_DrawPageIndicator(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    // KaleidoScope_SwitchPage raises unk_1E4 on the frame the cube starts turning.
    if (!OotmmSession_IsActive() || pauseCtx->pageIndex != PAUSE_ITEM || pauseCtx->state != 6 ||
        pauseCtx->unk_1E4 != 0) {
        return;
    }

    const size_t pages = TotalPages();
    if (pages < 2) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx* polyOpa = POLY_OPA_DISP;
    Gfx* gfx = Graph_GfxPlusOne(polyOpa);
    gSPDisplayList(OVERLAY_DISP++, gfx);

    GfxPrint printer;
    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);
    // The page background's top right corner sits at roughly column 35, row 5.
    GfxPrint_SetColor(&printer, 255, 255, 255, pauseCtx->alpha);
    GfxPrint_SetPos(&printer, 27, 6);
    GfxPrint_Printf(&printer, "%d/%d (L)", static_cast<int>(sPage + 1), static_cast<int>(pages));

    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    Graph_BranchDlist(polyOpa, gfx);
    POLY_OPA_DISP = gfx;

    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" int OotmmItemPage_PageCount(void) {
    return static_cast<int>(TotalPages());
}

extern "C" int OotmmItemPage_CurrentPage(void) {
    return static_cast<int>(sPage);
}
