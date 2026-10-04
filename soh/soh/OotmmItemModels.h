#pragma once

#include <libultraship/bridge/OotmmInventory.h>

#include <string>

struct PlayState;

void OotmmItemModels_Init();
bool OotmmItemModel_Draw(PlayState* play, const Ship::OotmmItemDefinition& item);
bool OotmmItemModel_DrawById(PlayState* play, const std::string& itemId, const std::string& itemName);
