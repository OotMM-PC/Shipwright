#include "OotmmSongEvents.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "OotmmSongs.h"
#include "OotmmSongsPlayer.h"

extern OcarinaSongInfo gOcarinaSongNotes[];
extern OcarinaSongInfo sOotmmSongNotes[];
}

namespace {

int32_t SongEventId(const Actor* actor, const PlayState* play) {
    if (actor == nullptr || play == nullptr) {
        return -1;
    }
    switch (play->sceneNum) {
        case SCENE_TEMPLE_OF_TIME:
            return 0;
        case SCENE_WINDMILL_AND_DAMPES_GRAVE:
            return 1;
        case SCENE_GRAVEYARD:
            return 2;
        case SCENE_ZORAS_RIVER:
            return 3;
        case SCENE_GORON_CITY:
            return 4;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS:
            return play->curSpawn <= 2 ? 5 + play->curSpawn : -1;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC:
            return play->curSpawn <= 2 ? 8 + play->curSpawn : -1;
        case SCENE_WATER_TEMPLE:
            return 11;
        case SCENE_SHADOW_TEMPLE:
            return 12;
        case SCENE_SPIRIT_TEMPLE:
            if (actor->room == 5) {
                return 13;
            }
            if (actor->room == 14) {
                return 14;
            }
            if (actor->room == 24) {
                return 15;
            }
            return -1;
        case SCENE_BOTTOM_OF_THE_WELL:
            return 16;
        case SCENE_INSIDE_GANONS_CASTLE:
            return 17;
        default:
            return -1;
    }
}

int32_t AssignedSong(const Actor* actor, const PlayState* play) {
    const int32_t eventId = SongEventId(actor, play);
    const auto& events = OotmmSession_GetState().GetSongEventsOot();
    if (eventId < 0 || static_cast<size_t>(eventId) >= events.size()) {
        return -1;
    }
    const int32_t song = events[eventId];
    return song >= 0 && song < 20 ? song : -1;
}

int32_t MmSongFor(int32_t song) {
    switch (song) {
        case 12:
            return OOTMM_SONG_HEALING;
        case 13:
            return OOTMM_SONG_SOARING;
        case 14:
            return OOTMM_SONG_AWAKENING;
        case 15:
            return OOTMM_SONG_GORON;
        case 16:
            return OOTMM_SONG_GORON_HALF;
        case 17:
            return OOTMM_SONG_ZORA;
        case 18:
            return OOTMM_SONG_ELEGY;
        case 19:
            return OOTMM_SONG_ORDER;
        default:
            return OOTMM_SONG_NONE;
    }
}

} // namespace

int32_t OotmmSongEvents_Enabled(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const auto& state = OotmmSession_GetState();
    return state.GetBoolSetting("songEventsShuffleOot", false) &&
           state.GetSongEventsOot().size() >= 18;
}

int32_t OotmmSongEvents_Action(Actor* actor, PlayState* play, int32_t vanillaAction) {
    if (!OotmmSongEvents_Enabled()) {
        return vanillaAction;
    }
    const int32_t song = AssignedSong(actor, play);
    if (song < 0) {
        return vanillaAction;
    }

    static constexpr int32_t kActions[] = {
        OCARINA_ACTION_CHECK_LULLABY,       OCARINA_ACTION_CHECK_EPONA,
        OCARINA_ACTION_CHECK_SARIA,         OCARINA_ACTION_CHECK_STORMS,
        OCARINA_ACTION_CHECK_SUNS,          OCARINA_ACTION_CHECK_TIME,
        OCARINA_ACTION_CHECK_MINUET,        OCARINA_ACTION_CHECK_BOLERO,
        OCARINA_ACTION_CHECK_SERENADE,      OCARINA_ACTION_CHECK_REQUIEM,
        OCARINA_ACTION_CHECK_NOCTURNE,      OCARINA_ACTION_CHECK_PRELUDE,
        OCARINA_ACTION_CHECK_OOTMM_HEALING, OCARINA_ACTION_CHECK_OOTMM_SOARING,
        OCARINA_ACTION_CHECK_OOTMM_AWAKENING, OCARINA_ACTION_CHECK_OOTMM_GORON,
        OCARINA_ACTION_CHECK_OOTMM_GORON_HALF, OCARINA_ACTION_CHECK_OOTMM_ZORA,
        OCARINA_ACTION_CHECK_OOTMM_ELEGY,   OCARINA_ACTION_CHECK_OOTMM_ORDER,
    };
    return kActions[song];
}

int32_t OotmmSongEvents_Song(Actor* actor, PlayState* play) {
    return OotmmSongEvents_Enabled() ? AssignedSong(actor, play) : -1;
}

int32_t OotmmSongEvents_GuideNotes(PlayState* play, uint8_t* notes, int32_t max) {
    if (play == nullptr || !OotmmSongEvents_Enabled()) {
        return 0;
    }

    const int32_t action = play->msgCtx.ocarinaAction;
    const OcarinaSongInfo* info = nullptr;
    if (action >= OCARINA_ACTION_CHECK_MINUET && action <= OCARINA_ACTION_CHECK_STORMS) {
        const int32_t songIdx = action - OCARINA_ACTION_CHECK_MINUET;
        if (CHECK_QUEST_ITEM(QUEST_SONG_MINUET + gOcarinaSongItemMap[songIdx])) {
            info = &gOcarinaSongNotes[songIdx];
        }
    } else if (action >= OCARINA_ACTION_CHECK_OOTMM_HEALING && action <= OCARINA_ACTION_CHECK_OOTMM_ORDER) {
        const int32_t song = MmSongFor(12 + (action - OCARINA_ACTION_CHECK_OOTMM_HEALING));
        if (song != OOTMM_SONG_NONE && (OotmmSongs_AvailableMask() & (1u << song))) {
            info = &sOotmmSongNotes[song - 1];
        }
    }
    if (info == nullptr) {
        return 0;
    }

    const int32_t len = info->len < max ? info->len : max;
    for (int32_t i = 0; i < len; i++) {
        notes[i] = info->notesIdx[i];
    }
    return len;
}

int32_t OotmmSongEvents_HandleCheckPlayed(PlayState* play) {
    const int32_t action = play->msgCtx.ocarinaAction;
    if (action >= OCARINA_ACTION_CHECK_OOTMM_HEALING && action <= OCARINA_ACTION_CHECK_OOTMM_ORDER) {
        const int32_t target = MmSongFor(12 + (action - OCARINA_ACTION_CHECK_OOTMM_HEALING));
        const int32_t played = OotmmSongs_Played();
        if (played == target || (target == OOTMM_SONG_GORON_HALF && played == OOTMM_SONG_GORON)) {
            OotmmSongs_ClearPlayed();
            play->msgCtx.ocarinaMode = OCARINA_MODE_03;
            return 1;
        }
    }
    return 0;
}
