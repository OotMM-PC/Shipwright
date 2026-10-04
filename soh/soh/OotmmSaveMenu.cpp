#include "OotmmSaveMenu.h"
#include "OotmmSession.h"
#include "SaveManager.h"
#include "Enhancements/custom-message/CustomMessageManager.h"
#include <libultraship/libultraship.h>
#include <string>

extern "C" {
#include "variables.h"
#include "z64.h"
#include "macros.h"
#include "functions.h"
}

namespace {
constexpr char kThreeWayChoice[] = "\x1C";
constexpr uint16_t kMenuTextId = 0x88C;
bool sActive = false;
bool sAwaitingSave = false;
bool sSaveSucceeded = false;

void ShowText(PlayState* play, const std::string& text) {
    Message_StartTextbox(play, kMenuTextId, nullptr);
    CustomMessage message(text);
    message.Format();
    message.LoadIntoFont();
}

void CloseMenu(PlayState* play) {
    sActive = false;
    Message_CloseTextbox(play);
    Audio_PlaySoundGeneral(NA_SE_SY_DECIDE, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                          &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}
}

extern "C" void OotmmSaveMenu_Open(struct PlayState* play) {
    if (!OotmmSession_IsActive() || play == nullptr) {
        return;
    }
    sActive = true;
    sAwaitingSave = true;
    ShowText(play, "Saving...");
}

extern "C" int OotmmSaveMenu_Active(void) {
    return sActive ? 1 : 0;
}

extern "C" int OotmmSaveMenu_Update(struct PlayState* play) {
    if (!sActive) {
        return 1;
    }
    if (sAwaitingSave) {
        if (SaveManager::Instance->IsSaving()) {
            Message_Update(play);
            return 0;
        }
        sSaveSucceeded = SaveManager::Instance->LastBaseSaveSucceeded(gSaveContext.fileNum);
        sAwaitingSave = false;
        ShowText(play, std::string(sSaveSucceeded ? "Game saved.&%g" : "Save failed. Previous save kept.&%g") +
                       kThreeWayChoice + (sSaveSucceeded ? "Resume&Return to Spawn&Quit Game"
                                                        : "Retry&Resume&Quit without saving"));
        return 0;
    }
    const auto buttons = play->state.input[0].press.button;
    if (CHECK_BTN_ALL(buttons, BTN_B) || CHECK_BTN_ALL(buttons, BTN_START)) {
        CloseMenu(play);
        return 1;
    }
    if (Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE || !CHECK_BTN_ALL(buttons, BTN_A)) {
        Message_Update(play);
        return 0;
    }
    const uint8_t choice = play->msgCtx.choiceIndex;
    if (!sSaveSucceeded && choice == 0) {
        Play_PerformSave(play);
        sAwaitingSave = true;
        ShowText(play, "Saving...");
        return 0;
    }
    CloseMenu(play);
    if (choice == 2) {
        Ship::Context::GetInstance()->GetWindow()->Close();
    } else if (sSaveSucceeded && choice == 1) {
        OotmmSession_ReturnToSpawn();
    }
    return 1;
}
