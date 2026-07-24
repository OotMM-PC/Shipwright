#include "OotmmOcarinaButtons.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

#include <string>
#include <string_view>

namespace {

bool ButtonShuffleActive() {
    return OotmmSession_IsActive() &&
           OotmmSession_GetState().GetBoolSetting("ocarinaButtonsShuffleOot", false);
}

bool OwnsButton(std::string_view suffix) {
    const auto& state = OotmmSession_GetState();
    const std::string prefix =
        state.GetBoolSetting("sharedOcarinaButtons", false) ? "SHARED_BUTTON_" : "OOT_BUTTON_";
    return OotmmIpc_GetInventory().Has(prefix + std::string(suffix));
}

} // namespace

extern "C" int32_t Ootmm_IsOcarinaButtonAvailable(int32_t button) {
    if (!ButtonShuffleActive()) {
        return true;
    }

    switch (button) {
        case OOTMM_OCARINA_BUTTON_A:
            return OwnsButton("A");
        case OOTMM_OCARINA_BUTTON_C_DOWN:
            return OwnsButton("C_DOWN");
        case OOTMM_OCARINA_BUTTON_C_RIGHT:
            return OwnsButton("C_RIGHT");
        case OOTMM_OCARINA_BUTTON_C_LEFT:
            return OwnsButton("C_LEFT");
        case OOTMM_OCARINA_BUTTON_C_UP:
            return OwnsButton("C_UP");
        default:
            return true;
    }
}

extern "C" void Ootmm_GateOcarinaButtonMaps(int32_t* d4, int32_t* d5, int32_t* b4, int32_t* a4,
                                             int32_t* f4) {
    if (!ButtonShuffleActive()) {
        return;
    }
    if (d4 != nullptr && !Ootmm_IsOcarinaButtonAvailable(OOTMM_OCARINA_BUTTON_A)) {
        *d4 = 0;
    }
    if (d5 != nullptr && !Ootmm_IsOcarinaButtonAvailable(OOTMM_OCARINA_BUTTON_C_UP)) {
        *d5 = 0;
    }
    if (b4 != nullptr && !Ootmm_IsOcarinaButtonAvailable(OOTMM_OCARINA_BUTTON_C_LEFT)) {
        *b4 = 0;
    }
    if (a4 != nullptr && !Ootmm_IsOcarinaButtonAvailable(OOTMM_OCARINA_BUTTON_C_RIGHT)) {
        *a4 = 0;
    }
    if (f4 != nullptr && !Ootmm_IsOcarinaButtonAvailable(OOTMM_OCARINA_BUTTON_C_DOWN)) {
        *f4 = 0;
    }
}
