#include "OotmmSongPage.h"

#include "OotmmItemPage.h"
#include "OotmmSession.h"
#include "OotmmSongs.h"
#include "ResourceManagerHelpers.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include <cstring>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
}

namespace {

constexpr int kSongSlots = QUEST_KOKIRI_EMERALD - QUEST_SONG_MINUET;
constexpr int kPauseStateMain = 6;
constexpr int kMainStateIdle = 0;
constexpr int kMainStateCursorOnSong = 8;
constexpr int kIndicatorColumn = 27;
constexpr int kIndicatorRow = 6;

constexpr int32_t kSlotSongs[kSongSlots] = {
    OOTMM_SONG_AWAKENING, OOTMM_SONG_GORON,   OOTMM_SONG_ZORA,        OOTMM_SONG_ELEGY,
    OOTMM_SONG_ORDER,     OOTMM_SONG_NONE,    OOTMM_SONG_NONE,        OOTMM_SONG_HEALING,
    OOTMM_SONG_NONE,      OOTMM_SONG_SOARING, OOTMM_SONG_NONE,        OOTMM_SONG_NONE,
};

constexpr const char* kNameTextures[OOTMM_SONG_COUNT] = {
    "__OTR__mm_item_name_static/gItemNameSongOfSoaringENGTex",
    "__OTR__mm_item_name_static/gItemNameElegyOfEmptynessENGTex",
    nullptr,
    "__OTR__mm_item_name_static/gItemNameSongOfHealingENGTex",
    "__OTR__mm_item_name_static/gItemNameSonataOfAwakeningENGTex",
    "__OTR__mm_item_name_static/gItemNameGoronLullabyENGTex",
    "__OTR__mm_item_name_static/gItemNameLullabyIntroENGTex",
    "__OTR__mm_item_name_static/gItemNameNewWaveBossaNovaENGTex",
    "__OTR__mm_item_name_static/gItemNameOathToOrderENGTex",
};

bool sSecondPage = false;

bool IsSongPoint(int point) {
    return point >= QUEST_SONG_MINUET && point < QUEST_KOKIRI_EMERALD;
}

bool Browsing(const PauseContext* pauseCtx) {
    return OotmmSession_IsActive() && pauseCtx->pageIndex == PAUSE_QUEST && pauseCtx->state == kPauseStateMain &&
           (pauseCtx->unk_1E4 == kMainStateIdle || pauseCtx->unk_1E4 == kMainStateCursorOnSong) &&
           pauseCtx->debugState == 0;
}

bool CursorOnSong(const PauseContext* pauseCtx) {
    return pauseCtx->cursorSpecialPos == 0 && IsSongPoint(pauseCtx->cursorSlot[PAUSE_QUEST]);
}

const char* NameTexture(int32_t song) {
    if (song == OOTMM_SONG_NONE) {
        return nullptr;
    }
    const char* name = kNameTextures[song - 1];
    return name != nullptr && ResourceMgr_FileExists(name) ? name : nullptr;
}

uint16_t PageItem(int point) {
    const bool named = sSecondPage ? NameTexture(OotmmSongPage_SlotSong(point - QUEST_SONG_MINUET)) != nullptr
                                   : CHECK_QUEST_ITEM(point) != 0;
    return named ? static_cast<uint16_t>(ITEM_SONG_MINUET + (point - QUEST_SONG_MINUET)) : PAUSE_ITEM_NONE;
}

bool HasSecondPageSongs() {
    for (int slot = 0; slot < kSongSlots; slot++) {
        if (OotmmSongPage_SlotSong(slot) != OOTMM_SONG_NONE) {
            return true;
        }
    }
    return false;
}

void ShowPage(PauseContext* pauseCtx, bool secondPage) {
    const int point = pauseCtx->cursorPoint[PAUSE_QUEST];

    sSecondPage = secondPage;
    if (pauseCtx->pageIndex == PAUSE_QUEST) {
        pauseCtx->namedItem = PAUSE_ITEM_NONE;
    }
    if (pauseCtx->unk_1E4 == kMainStateCursorOnSong) {
        pauseCtx->unk_1E4 = kMainStateIdle;
    }
    if (IsSongPoint(point)) {
        pauseCtx->cursorItem[PAUSE_QUEST] = PageItem(point);
    }
}

} // namespace

extern "C" void OotmmSongPage_Init(void) {
    REGISTER_VB_SHOULD(VB_DRAW_CUSTOM_ITEM_NAME, {
        const char* name = gPlayState != nullptr ? NameTexture(OotmmSongPage_HoverSong(gPlayState)) : nullptr;
        if (name != nullptr) {
            std::memcpy(gPlayState->pauseCtx.nameSegment, name, std::strlen(name) + 1);
            *should = true;
        }
    });
}

extern "C" int OotmmSongPage_Active(void) {
    if (!sSecondPage) {
        return 0;
    }
    if (gPlayState == nullptr) {
        sSecondPage = false;
    } else if (!OotmmSession_IsActive() || gPlayState->pauseCtx.state != kPauseStateMain ||
               gPlayState->pauseCtx.pageIndex != PAUSE_QUEST) {
        ShowPage(&gPlayState->pauseCtx, false);
    }
    return sSecondPage;
}

extern "C" int OotmmSongPage_ConsumePageToggle(void) {
    const bool secondPage = OotmmSongPage_Active();

    if (gPlayState == nullptr) {
        return 0;
    }
    PauseContext* pauseCtx = &gPlayState->pauseCtx;
    if (!Browsing(pauseCtx) || !CursorOnSong(pauseCtx) ||
        !CHECK_BTN_ALL(gPlayState->state.input[0].press.button, BTN_L)) {
        return 0;
    }
    if (!secondPage && !HasSecondPageSongs()) {
        return 0;
    }

    ShowPage(pauseCtx, !secondPage);
    Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    return 1;
}

extern "C" int32_t OotmmSongPage_SlotSong(int slot) {
    if (slot < 0 || slot >= kSongSlots) {
        return OOTMM_SONG_NONE;
    }
    const uint16_t enabled = OotmmSongs_EnabledMask();
    int32_t song = kSlotSongs[slot];
    if (song == OOTMM_SONG_GORON && !(enabled & (1 << OOTMM_SONG_GORON))) {
        song = OOTMM_SONG_GORON_HALF;
    }
    return song != OOTMM_SONG_NONE && (enabled & (1 << song)) ? song : OOTMM_SONG_NONE;
}

extern "C" int32_t OotmmSongPage_HoverSong(PlayState* play) {
    const PauseContext* pauseCtx = &play->pauseCtx;

    if (!OotmmSongPage_Active() || pauseCtx->cursorSpecialPos != 0) {
        return OOTMM_SONG_NONE;
    }
    return OotmmSongPage_SlotSong(pauseCtx->cursorSlot[PAUSE_QUEST] - QUEST_SONG_MINUET);
}

extern "C" uint16_t OotmmSongPage_CursorItem(PlayState* play, uint16_t nativeItem) {
    const int point = play->pauseCtx.cursorPoint[PAUSE_QUEST];

    return OotmmSongPage_Active() && IsSongPoint(point) ? PageItem(point) : nativeItem;
}

extern "C" void OotmmSongPage_EnterHover(PlayState* play) {
    if (OotmmSongPage_HoverSong(play) == OOTMM_SONG_NONE) {
        return;
    }

    VREG(21) = -62;
    VREG(22) = -56;
    VREG(23) = -49;
    VREG(24) = -46;
    VREG(25) = -41;
    play->pauseCtx.unk_1E4 = kMainStateCursorOnSong;
}

extern "C" void OotmmSongPage_DrawPageIndicator(PlayState* play) {
    const PauseContext* pauseCtx = &play->pauseCtx;

    if (!Browsing(pauseCtx)) {
        return;
    }

    const bool secondPage = OotmmSongPage_Active();
    if (!secondPage && (!CursorOnSong(pauseCtx) || !HasSecondPageSongs())) {
        return;
    }
    OotmmItemPage_DrawPageNumber(play, secondPage ? 2 : 1, 2, kIndicatorColumn, kIndicatorRow);
}
