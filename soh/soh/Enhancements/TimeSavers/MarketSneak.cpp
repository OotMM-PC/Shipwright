#include <soh/OTRGlobals.h>
#include <soh/ShipInit.hpp>
#include <soh/Enhancements/custom-message/CustomMessageManager.h>
#include <soh/Enhancements/custom-message/CustomMessageTypes.h>
#include <soh/Enhancements/game-interactor/GameInteractor.h>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include <variables.h>
extern PlayState* gPlayState;
}

// TODO: Port the remaining Market Sneak behavior.

void BuildNightGuardMessage(uint16_t* textId, bool* loadFromMessageTable) {
    // Other guards should not have their text overridden
    if (gPlayState->sceneNum != SCENE_MARKET_ENTRANCE_NIGHT) {
        return;
    }

    CustomMessage msg = CustomMessage("You look bored. Wanna go out for a walk?\x1B%gYes&No%w",
                                      "Du siehst gelangweilt aus. Willst Du einen Spaziergang machen?\x1B%gJa&Nein%w",
                                      "Tu as l'air de t'ennuyer. Tu veux aller faire un tour?\x1B%gOui&Non%w");
    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void MarketSneak_Register() {
    COND_ID_HOOK(OnOpenText, TEXT_MARKET_GUARD_NIGHT, CVarGetInteger(CVAR_ENHANCEMENT("MarketSneak"), 0),
                 BuildNightGuardMessage);
}

static RegisterShipInitFunc initFunc(MarketSneak_Register, { CVAR_ENHANCEMENT("MarketSneak") });
