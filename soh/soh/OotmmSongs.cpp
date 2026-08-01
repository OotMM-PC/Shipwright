#include "OotmmSongs.h"

#include "Enhancements/audio/AudioCollection.h"
#include "Enhancements/custom-message/CustomMessageManager.h"
#include "OotmmIpc.h"
#include "OotmmSession.h"
#include "SaveManager.h"
#include "cvar_prefixes.h"

#include <array>
#include <string>
#include <vector>

#include "global.h"

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

#include "OotmmSoaringMap.h"
#include "OotmmSongsPlayer.h"

extern PlayState* gPlayState;
extern f32 sFontWidths[144];
}

namespace {

struct SongDef {
    const char* Name;
    const char* ItemId;
    const char* SharedItemId;
    const char* NoteItemId;
    const char* SharedNoteItemId;
    uint32_t NotesNeeded;
    const char* FanfareLabel;
};

constexpr std::array<SongDef, OOTMM_SONG_COUNT> kSongs = { {
    { "Song of Soaring", "OOT_SONG_SOARING", "SHARED_SONG_SOARING", "OOT_SONG_NOTE_SOARING",
      "SHARED_SONG_NOTE_SOARING", 6, "SongOfSoaring" },
    { "Elegy of Emptiness", "OOT_SONG_EMPTINESS", "SHARED_SONG_EMPTINESS", "OOT_SONG_NOTE_EMPTINESS",
      "SHARED_SONG_NOTE_EMPTINESS", 7, "ElegyOfEmptiness" },
    { "Song of Double Time", nullptr, nullptr, nullptr, nullptr, 0, "SongOfDoubleTime" },
    { "Song of Healing", "OOT_SONG_HEALING", "SHARED_SONG_HEALING", "OOT_SONG_NOTE_HEALING",
      "SHARED_SONG_NOTE_HEALING", 6, "SongOfHealing" },
    { "Sonata of Awakening", "OOT_SONG_AWAKENING", "SHARED_SONG_AWAKENING", "OOT_SONG_NOTE_AWAKENING",
      "SHARED_SONG_NOTE_AWAKENING", 7, "SonataOfAwakening" },
    { "Goron Lullaby", "OOT_SONG_GORON", "SHARED_SONG_GORON", "OOT_SONG_NOTE_GORON", "SHARED_SONG_NOTE_GORON",
      8, "GoronLullaby" },
    { "Goron Lullaby Intro", "OOT_SONG_GORON_HALF", "SHARED_SONG_GORON_HALF", "OOT_SONG_NOTE_GORON",
      "SHARED_SONG_NOTE_GORON", 6, "GoronLullabyIntro" },
    { "New Wave Bossa Nova", "OOT_SONG_ZORA", "SHARED_SONG_ZORA", "OOT_SONG_NOTE_ZORA", "SHARED_SONG_NOTE_ZORA",
      7, "NewWaveBossaNova" },
    { "Oath to Order", "OOT_SONG_ORDER", "SHARED_SONG_ORDER", "OOT_SONG_NOTE_ORDER", "SHARED_SONG_NOTE_ORDER",
      6, "OathToOrder" },
} };

struct OwlDef {
    const char* Entrance;
    uint32_t NativeId;
    const char* ItemId;
    const char* PreActivatedFlag;
    const char* Name;
};

constexpr int32_t OOTMM_OWL_CLOCK_TOWN = 4;

constexpr std::array<OwlDef, 10> kOwls = { {
    { "MM_WARP_OWL_GREAT_BAY", 0x68B0, "MM_OWL_GREAT_BAY", "greatbay", "Great Bay Coast" },
    { "MM_WARP_OWL_ZORA_CAPE", 0x6A60, "MM_OWL_ZORA_CAPE", "zoracape", "Zora Cape" },
    { "MM_WARP_OWL_SNOWHEAD", 0xB230, "MM_OWL_SNOWHEAD", "snowhead", "Snowhead" },
    { "MM_WARP_OWL_MOUNTAIN_VILLAGE", 0x9A80, "MM_OWL_MOUNTAIN_VILLAGE", "mountain", "Mountain Village" },
    { "MM_WARP_OWL_CLOCK_TOWN", 0xD890, "MM_OWL_CLOCK_TOWN", "clocktown", "Clock Town" },
    { "MM_WARP_OWL_MILK_ROAD", 0x3E40, "MM_OWL_MILK_ROAD", "milkroad", "Milk Road" },
    { "MM_WARP_OWL_WOODFALL", 0x8640, "MM_OWL_WOODFALL", "woodfall", "Woodfall" },
    { "MM_WARP_OWL_SOUTHERN_SWAMP", 0x84A0, "MM_OWL_SOUTHERN_SWAMP", "swamp", "Southern Swamp" },
    { "MM_WARP_OWL_IKANA_CANYON", 0x2040, "MM_OWL_IKANA_CANYON", "canyon", "Ikana Canyon" },
    { "MM_WARP_OWL_STONE_TOWER", 0xAA30, "MM_OWL_STONE_TOWER", "tower", "Stone Tower" },
} };

constexpr int32_t kHandoffHoldFrames = 120;
constexpr int32_t kHandoffLatchFrames = 1200;

uint8_t sPlayedSong = OOTMM_SONG_NONE;
uint8_t sActiveSong = OOTMM_SONG_NONE;
bool sMenuOpen = false;
bool sMenuHoldsPlayer = false;
int32_t sMenuCursor = 0;
int32_t sMenuStickHeld = 0;
int32_t sHandoffFrames = -1;
bool sDoubleTimeAsked = false;
std::vector<uint8_t> sMenuEntries;

const SongDef& Def(int32_t song) {
    return kSongs[song - 1];
}

bool OwnsSong(int32_t song) {
    const SongDef& def = Def(song);
    if (def.ItemId == nullptr) {
        return false;
    }
    const auto& inventory = OotmmIpc_GetInventory();
    if (inventory.Has(def.ItemId) || inventory.Has(def.SharedItemId)) {
        return true;
    }
    if (song == OOTMM_SONG_GORON_HALF &&
        OotmmSession_GetState().GetStringSetting("progressiveGoronLullabyOot", "progressive") != "progressive") {
        return false;
    }
    const uint32_t notes = inventory.Count(def.NoteItemId) + inventory.Count(def.SharedNoteItemId);
    return notes >= def.NotesNeeded;
}

bool SongEnabled(int32_t song) {
    if (song == OOTMM_SONG_DOUBLE_TIME) {
        return CHECK_QUEST_ITEM(QUEST_SONG_TIME) &&
               OotmmSession_GetState().GetBoolSetting("songOfDoubleTimeOot", false);
    }
    return OwnsSong(song);
}

bool SongAvailable(int32_t song) {
    if (!SongEnabled(song)) {
        return false;
    }
    switch (song) {
        case OOTMM_SONG_SOARING:
            return LINK_IS_CHILD || OotmmSession_GetState().GetBoolSetting("agelessSoaring", false);
        case OOTMM_SONG_GORON_HALF:
            return !OwnsSong(OOTMM_SONG_GORON);
        default:
            return true;
    }
}

uint16_t MaskFor(bool (*test)(int32_t)) {
    uint16_t mask = 0;
    for (int32_t song = OOTMM_SONG_NONE + 1; song < OOTMM_SONG_MAX; song++) {
        if (test(song)) {
            mask |= static_cast<uint16_t>(1 << song);
        }
    }
    return mask;
}

bool OwlActivated(const OwlDef& owl) {
    if (OotmmSession_GetState().GetStringSetting("owlShuffle", "none") != "anywhere") {
        return true;
    }
    return OotmmIpc_GetInventory().Has(owl.ItemId) ||
           OotmmSession_GetState().WorldFlagContains("mmPreActivatedOwls", owl.PreActivatedFlag);
}

void BuildMenuEntries() {
    sMenuEntries.clear();
    for (int32_t i = 0; i < static_cast<int32_t>(kOwls.size()); i++) {
        if (OwlActivated(kOwls[i])) {
            sMenuEntries.push_back(static_cast<uint8_t>(i));
        }
    }
}

uint8_t CenteringShift(const std::string& line) {
    const f32 charScale = R_TEXT_CHAR_SCALE / 100.0f;
    const f32 spacing = (f32)CVarGetInteger(CVAR_ENHANCEMENT("TextSpacing"), 6);

    f32 width = 0.0f;
    for (const char c : line) {
        width += c == ' ' ? spacing : sFontWidths[c - ' '] * charScale;
    }

    const f32 shift = R_TEXTBOX_X + R_TEXTBOX_WIDTH / 2.0f - R_TEXT_INIT_XPOS - width / 2.0f;
    return shift <= 0.0f ? 0 : (shift >= 127.0f ? 127 : (uint8_t)shift);
}

void LoadMessage(const std::string& english) {
    CustomMessage message(english);
    message.AutoFormat();
    message.LoadIntoFont();
}

void ReleasePlayer(PlayState* play) {
    if (sMenuHoldsPlayer) {
        sMenuHoldsPlayer = false;
        GET_PLAYER(play)->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
    }
}

void EndOcarina(PlayState* play) {
    sActiveSong = OOTMM_SONG_NONE;
    sMenuOpen = false;
    sDoubleTimeAsked = false;
    ReleasePlayer(play);
    play->msgCtx.ocarinaMode = OCARINA_MODE_04;
}

Ship::OotmmEntranceMapping ResolveOwl(const OwlDef& owl) {
    const Ship::OotmmEntranceMapping* mapping =
        OotmmSession_GetState().FindEntrance(Ship::OotmmGame::Mm, owl.NativeId);

    Ship::OotmmEntranceMapping request;
    request.FromGame = Ship::OotmmGame::Oot;
    request.From = owl.Entrance;
    request.ToGame = mapping != nullptr ? mapping->ToGame : Ship::OotmmGame::Mm;
    request.To = mapping != nullptr ? mapping->To : owl.Entrance;
    request.ToNativeId = mapping != nullptr && mapping->ToNativeId.has_value() ? *mapping->ToNativeId : owl.NativeId;
    return request;
}

bool OwlLeavesOot(const OwlDef& owl) {
    return ResolveOwl(owl).ToGame == Ship::OotmmGame::Mm;
}

bool WarpToOwl(PlayState* play, const OwlDef& owl) {
    const Ship::OotmmEntranceMapping request = ResolveOwl(owl);

    if (request.ToGame == Ship::OotmmGame::Oot) {
        play->nextEntranceIndex = static_cast<int16_t>(*request.ToNativeId);
        play->transitionTrigger = TRANS_TRIGGER_START;
        play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
        gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
        return true;
    }

    return OotmmIpc_SendCrossGameTransition(request, static_cast<uint32_t>(gSaveContext.linkAge));
}

void SetupSoaring(PlayState* play) {
    if (play->msgCtx.disableWarpSongs || play->interfaceCtx.restrictions.warpSongs == 3) {
        Message_StartTextbox(play, 0x88C, NULL);
        EndOcarina(play);
        return;
    }

    BuildMenuEntries();
    if (sMenuEntries.empty()) {
        Message_StartTextbox(play, 0x88C, NULL);
        LoadMessage("You have yet to leave your %rmark%w&on any of the %rstatues%w you have&come across.");
        EndOcarina(play);
        return;
    }

    sMenuOpen = true;
    sMenuCursor = 0;
    for (int32_t i = 0; i < static_cast<int32_t>(sMenuEntries.size()); i++) {
        if (sMenuEntries[i] == OOTMM_OWL_CLOCK_TOWN) {
            sMenuCursor = i;
        }
    }
    sMenuStickHeld = 0;
    sActiveSong = OOTMM_SONG_SOARING;

    Player* player = GET_PLAYER(play);
    if (!(player->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE)) {
        player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        sMenuHoldsPlayer = true;
    }
}

void CommitHandoff(PlayState* play, Player* player) {
    sHandoffFrames = -1;
    sActiveSong = OOTMM_SONG_NONE;
    sMenuOpen = false;
    ReleasePlayer(play);
    Play_PerformSave(play);
    SaveManager::Instance->ThreadPoolWait();
    player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
    player->actor.freezeTimer = 10000;
}

void AbandonHandoff(PlayState* play, Player* player) {
    sActiveSong = OOTMM_SONG_NONE;
    player->stateFlags1 &= ~PLAYER_STATE1_IN_CUTSCENE;
    Message_StartTextbox(play, 0x88C, NULL);
    LoadMessage("The %rowl statues%w did not answer&your call.");
    play->msgCtx.ocarinaMode = OCARINA_MODE_04;
}

bool AwaitHandoff(PlayState* play, Player* player) {
    if (OotmmIpc_ConsumeTransitionAccepted()) {
        CommitHandoff(play, player);
        return true;
    }

    const int32_t frames = sHandoffFrames++;
    const bool connected = OotmmIpc_IsConnected();
    if (connected && frames < kHandoffHoldFrames) {
        // Player_SetupAction clears IN_CUTSCENE once ocarinaMode is 04, so the hold is re-asserted here.
        player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;
        return true;
    }

    if (frames < kHandoffHoldFrames) {
        sHandoffFrames = kHandoffHoldFrames + 1;
    }
    if (frames <= kHandoffHoldFrames) {
        AbandonHandoff(play, player);
    }
    if (!connected || sHandoffFrames >= kHandoffLatchFrames) {
        sHandoffFrames = -1;
    }
    return false;
}

void UpdateSoaring(PlayState* play, Player* player) {
    if (!sMenuOpen) {
        return;
    }

    Input* input = &play->state.input[0];
    int32_t step = 0;
    if (CHECK_BTN_ANY(input->press.button, BTN_DRIGHT | BTN_DDOWN)) {
        step = 1;
    } else if (CHECK_BTN_ANY(input->press.button, BTN_DLEFT | BTN_DUP)) {
        step = -1;
    }

    int32_t stick = 0;
    if (input->cur.stick_x > 30 || input->cur.stick_y < -30) {
        stick = 1;
    } else if (input->cur.stick_x < -30 || input->cur.stick_y > 30) {
        stick = -1;
    }
    if (stick != 0 && stick != sMenuStickHeld) {
        step = stick;
    }
    sMenuStickHeld = stick;

    const int32_t count = static_cast<int32_t>(sMenuEntries.size());
    if (step != 0) {
        sMenuCursor = (sMenuCursor + step + count) % count;
        OotmmSoaringMap_CursorMoved();
        Audio_PlaySoundGeneral(NA_SE_SY_CURSOR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    }

    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        Audio_PlaySoundGeneral(NA_SE_SY_ERROR, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        EndOcarina(play);
        return;
    }

    if (!CHECK_BTN_ALL(input->press.button, BTN_A)) {
        return;
    }

    const OwlDef& owl = kOwls[sMenuEntries[sMenuCursor]];
    const bool crossGame = OwlLeavesOot(owl);
    sMenuOpen = false;
    sActiveSong = OOTMM_SONG_NONE;
    ReleasePlayer(play);
    play->msgCtx.ocarinaMode = OCARINA_MODE_04;

    if (!WarpToOwl(play, owl)) {
        Message_StartTextbox(play, 0x88C, NULL);
        LoadMessage("The launcher is not listening, so&you cannot soar to %r" + std::string(owl.Name) + "%w.");
        play->msgCtx.ocarinaMode = OCARINA_MODE_04;
        return;
    }

    Audio_PlaySoundGeneral(NA_SE_SY_CORRECT_CHIME, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    player->stateFlags1 |= PLAYER_STATE1_IN_CUTSCENE;

    if (crossGame) {
        sActiveSong = OOTMM_SONG_SOARING;
        sHandoffFrames = 0;
    } else {
        player->actor.freezeTimer = 10000;
    }
}

bool DoubleTimeBlocked(PlayState* play) {
    if (gSaveContext.sunsSongState != SUNSSONG_INACTIVE) {
        return true;
    }
    return play->envCtx.timeIncrement == 0 &&
           (play->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_1 ||
            play->interfaceCtx.restrictions.sunsSong == 3);
}

void SetupDoubleTime(PlayState* play) {
    if (DoubleTimeBlocked(play)) {
        Message_StartTextbox(play, 0x88C, NULL);
        LoadMessage("Your notes echoed far....&but nothing happened.");
        EndOcarina(play);
        return;
    }

    const bool day = gSaveContext.dayTime >= CLOCK_TIME(6, 30) && gSaveContext.dayTime < CLOCK_TIME(18, 0) + 1;
    Message_StartTextbox(play, 0x00E0, NULL);
    LoadMessage(std::string("Proceed to %r") + (day ? "Dusk" : "Dawn") + "%w?" +
                CustomMessage::TWO_WAY_CHOICE() + "%gYes&No%w");
    sActiveSong = OOTMM_SONG_DOUBLE_TIME;
}

void ApplyDoubleTime(PlayState* play) {
    const bool day = gSaveContext.dayTime >= CLOCK_TIME(6, 30) && gSaveContext.dayTime < CLOCK_TIME(18, 0) + 1;
    const u16 target = day ? (u16)(CLOCK_TIME(18, 0) + 1) : (u16)CLOCK_TIME(6, 30);

    if (play->envCtx.timeIncrement != 0) {
        gSaveContext.dayTime = target;
        gSaveContext.skyboxTime = target;
        return;
    }

    if (play->roomCtx.curRoom.behaviorType1 == ROOM_BEHAVIOR_TYPE1_1 ||
        play->interfaceCtx.restrictions.sunsSong == 3) {
        return;
    }

    gSaveContext.nextDayTime = target;
    if (day) {
        play->transitionType = TRANS_TYPE_FADE_WHITE_FAST;
        gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
    } else {
        play->transitionType = TRANS_TYPE_FADE_BLACK_FAST;
        gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    }
    if (play->sceneNum == SCENE_HAUNTED_WASTELAND) {
        play->transitionType = TRANS_TYPE_SANDSTORM_PERSIST;
        gSaveContext.nextTransitionType = TRANS_TYPE_SANDSTORM_PERSIST;
    }

    play->unk_11DE9 = 1;
    gSaveContext.respawnFlag = -2;
    play->nextEntranceIndex = gSaveContext.entranceIndex;
    play->transitionTrigger = TRANS_TRIGGER_START;
    func_800F6964(30);
    gSaveContext.seqId = (u8)NA_BGM_DISABLED;
    gSaveContext.natureAmbienceId = NATURE_ID_DISABLED;
}

void UpdateDoubleTime(PlayState* play) {
    if (Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE) {
        if (sDoubleTimeAsked) {
            EndOcarina(play);
        }
        return;
    }
    sDoubleTimeAsked = true;
    if (!Message_ShouldAdvance(play)) {
        return;
    }

    const u8 choice = play->msgCtx.choiceIndex;
    Message_CloseTextbox(play);
    EndOcarina(play);
    if (choice == 0) {
        ApplyDoubleTime(play);
    }
}

} // namespace

extern "C" uint16_t OotmmSongs_AvailableMask(void) {
    if (!OotmmSession_IsActive() || gPlayState == nullptr) {
        return 0;
    }

    const u16 action = gPlayState->msgCtx.ocarinaAction;
    if (action != OCARINA_ACTION_FREE_PLAY && action != OCARINA_ACTION_CHECK_NOWARP) {
        return 0;
    }

    return MaskFor(SongAvailable);
}

extern "C" uint16_t OotmmSongs_ScarecrowBlockMask(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    return MaskFor(SongEnabled);
}

extern "C" int32_t OotmmSongs_FanfareSeqId(int32_t song) {
    if (song <= OOTMM_SONG_NONE || song >= OOTMM_SONG_MAX) {
        return -1;
    }
    return AudioCollection::Instance->GetSequenceNumByName(Def(song).FanfareLabel);
}

extern "C" int32_t OotmmSongs_MenuIsOpen(void) {
    return sMenuOpen ? 1 : 0;
}

extern "C" const char* OotmmSongs_Name(int32_t song) {
    if (song <= OOTMM_SONG_NONE || song >= OOTMM_SONG_MAX) {
        return nullptr;
    }
    return Def(song).Name;
}

extern "C" void OotmmSongs_NotePlayed(int32_t song) {
    sPlayedSong = static_cast<uint8_t>(song);
}

extern "C" int32_t OotmmSongs_Played(void) {
    return sPlayedSong;
}

extern "C" void OotmmSongs_ClearPlayed(void) {
    sPlayedSong = OOTMM_SONG_NONE;
}

extern "C" void OotmmSongs_LoadPlayedText(void) {
    if (sPlayedSong == OOTMM_SONG_NONE) {
        return;
    }

    const std::string line = "You played the " + std::string(Def(sPlayedSong).Name) + ".";

    CustomMessage body("You played the %r" + std::string(Def(sPlayedSong).Name) + "%w.&&&");
    body.Format();

    // Message_Decode only lifts the text clear of the note staff for vanilla 0x893's quick-text code
    // and its three trailing newlines. The header goes on after Format, which would rewrite a shift
    // byte of '&', '^' or '@'.
    CustomMessage message(std::string("\x08\x06") + static_cast<char>(CenteringShift(line)) +
                          body.GetEnglish(MF_RAW));
    message.LoadIntoFont();
}

extern "C" int32_t OotmmSongs_EffectActorId(void) {
    return sPlayedSong == OOTMM_SONG_NONE ? -1 : ACTOR_OCEFF_WIPE;
}

extern "C" uint16_t OotmmSongs_EffectActorParams(void) {
    return 1;
}

extern "C" void OotmmSongs_Begin(PlayState* play) {
    const int32_t song = sPlayedSong;
    sPlayedSong = OOTMM_SONG_NONE;
    sActiveSong = OOTMM_SONG_NONE;
    sMenuOpen = false;

    switch (song) {
        case OOTMM_SONG_SOARING:
            SetupSoaring(play);
            break;
        case OOTMM_SONG_DOUBLE_TIME:
            SetupDoubleTime(play);
            break;
        default:
            play->msgCtx.ocarinaMode = OCARINA_MODE_04;
            break;
    }
}

extern "C" void OotmmSongs_Update(PlayState* play, Player* player) {
    if (play == nullptr || player == nullptr) {
        return;
    }
    if (!OotmmSession_IsActive()) {
        sHandoffFrames = -1;
        if (sActiveSong != OOTMM_SONG_NONE) {
            EndOcarina(play);
        }
        return;
    }
    if (sHandoffFrames >= 0 && AwaitHandoff(play, player)) {
        return;
    }

    switch (sActiveSong) {
        case OOTMM_SONG_SOARING:
            UpdateSoaring(play, player);
            break;
        case OOTMM_SONG_DOUBLE_TIME:
            UpdateDoubleTime(play);
            break;
    }
}

extern "C" void OotmmSongs_DrawMenu(PlayState* play) {
    if (!sMenuOpen || play == nullptr || sMenuEntries.empty()) {
        return;
    }

    if (OotmmSoaringMap_Available()) {
        OotmmSoaringMap_Draw(play, sMenuEntries.data(), static_cast<int32_t>(sMenuEntries.size()), sMenuCursor);
        return;
    }

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    GfxPrint printer{};

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfxCtx->overlay.p);

    GfxPrint_SetColor(&printer, 255, 230, 120, 255);
    GfxPrint_SetPos(&printer, 4, 5);
    GfxPrint_Printf(&printer, "SOAR TO");

    for (int32_t i = 0; i < static_cast<int32_t>(sMenuEntries.size()); i++) {
        const bool selected = i == sMenuCursor;
        GfxPrint_SetColor(&printer, 255, 255, selected ? 130 : 255, selected ? 255 : 170);
        GfxPrint_SetPos(&printer, 4, 7 + i);
        GfxPrint_Printf(&printer, "%c %s", selected ? '>' : ' ', kOwls[sMenuEntries[i]].Name);
    }

    gfxCtx->overlay.p = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);
}
