#include "OotmmSpinUpgrade.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

namespace {

constexpr float kFullCharge = 1.0f;
constexpr float kHalfCharge = 0.5f;

bool ShuffleActive() {
    return OotmmSession_IsActive() &&
           OotmmSession_GetState().GetBoolSetting("spinUpgradeOot", false);
}

bool Owned() {
    const auto& state = OotmmSession_GetState();
    const auto& inventory = OotmmIpc_GetInventory();
    if (state.GetBoolSetting("sharedSpinUpgrade", false) &&
        inventory.Has("SHARED_SPIN_UPGRADE")) {
        return true;
    }
    return inventory.Has("OOT_SPIN_UPGRADE");
}

} // namespace

extern "C" float OotmmSpinUpgrade_ChargeLimit(void) {
    if (!ShuffleActive() || Owned()) {
        return kFullCharge;
    }
    return kHalfCharge;
}

extern "C" int32_t OotmmSpinUpgrade_SpinLevel(void) {
    if (!ShuffleActive()) {
        return -1;
    }
    return Owned() ? 1 : 0;
}
