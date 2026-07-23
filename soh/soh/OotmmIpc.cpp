#include "OotmmIpc.h"

#include "variables.h"

#include <libultraship/bridge/GameIpcSession.h>

namespace {

Ship::GameIpcSession sSession;
Ship::GameIpcSettings sSettings;

} // namespace

extern "C" int gOotmmGameSpeedPercent = 100;
extern "C" int gOotmmGameSpeedSmooth = 0;

void OotmmIpc_Init() {
    sSession.Start("oot", gBuildVersion);
}

void OotmmIpc_Pump() {
    if (sSession.Pump(sSettings)) {
        gOotmmGameSpeedPercent = sSettings.SpeedPercent;
        gOotmmGameSpeedSmooth = sSettings.SpeedSmooth ? 1 : 0;
        sSession.AcknowledgeSettings(sSettings.Revision);
    }

}

void OotmmIpc_Shutdown() {
    sSession.Stop();
}

bool OotmmIpc_SendCrossGameTransition(const Ship::OotmmEntranceMapping& mapping,
                                      std::optional<uint32_t> ootAge) {
    return sSession.RequestCrossGameTransition(
        "oot", mapping.ToGame == Ship::OotmmGame::Oot ? "oot" : "mm", mapping.To, mapping.ToNativeId, ootAge);
}

bool OotmmIpc_ConsumeTransitionAccepted() {
    return sSession.ConsumeTransitionAccepted();
}
