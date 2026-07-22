#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"

void RegisterNoMasterSword() {
}

static RegisterShipInitFunc initFunc(RegisterNoMasterSword);
