#include "OotmmSaveMenu.h"

#include "OotmmSession.h"

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

enum SaveMenuChoice {
    kResume,
    kReturnToSpawn,
    kQuitGame,
};

// Three-way choice control code; CustomMessage only names the two-way one.
constexpr char kThreeWayChoice[] = "\x1C";
constexpr uint16_t kMenuTextId = 0x88C;

bool sActive = false;

void LoadMenuText() {
    // The choice cursor rows are fixed, so exactly one break may precede them.
    CustomMessage message(std::string("Game saved.&%g") + kThreeWayChoice +
                          "Resume&Return to Spawn&Quit Game");
    // Format is what turns & into line breaks and terminates the message.
    message.Format();
    message.LoadIntoFont();
}

} // namespace

extern "C" void OotmmSaveMenu_Open(struct PlayState* play) {
    if (!OotmmSession_IsActive() || play == nullptr) {
        return;
    }
    Message_StartTextbox(play, kMenuTextId, nullptr);
    LoadMenuText();
    sActive = true;
}

extern "C" int OotmmSaveMenu_Active(void) {
    return sActive ? 1 : 0;
}

extern "C" int OotmmSaveMenu_Update(struct PlayState* play) {
    if (!sActive) {
        return 1;
    }
    if (Message_GetState(&play->msgCtx) != TEXT_STATE_CHOICE ||
        !CHECK_BTN_ALL(play->state.input[0].press.button, BTN_A)) {
        // Play_Update skips Message_Update while the pause menu is up, so the box only advances if
        // we pump it. Confirm is read above so this never consumes it.
        Message_Update(play);
        return 0;
    }

    const uint8_t choice = play->msgCtx.choiceIndex;
    sActive = false;
    Message_CloseTextbox(play);
    Audio_PlaySoundGeneral(NA_SE_SY_DECIDE, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);

    switch (choice) {
        case kReturnToSpawn:
            OotmmSession_ReturnToSpawn();
            break;
        case kQuitGame:
            Ship::Context::GetInstance()->GetWindow()->Close();
            break;
        default:
            break;
    }
    return 1;
}
