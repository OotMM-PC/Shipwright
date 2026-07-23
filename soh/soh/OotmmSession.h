#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <libultraship/bridge/OotmmGameState.h>

#include <string>

extern "C" {
#endif

int32_t OotmmSession_IsActive(void);
void OotmmSession_InitializeNewSave(void);

#ifdef __cplusplus
}

void OotmmSession_Init();
const Ship::OotmmGameState& OotmmSession_GetState();
std::string OotmmSession_GetSaveSubdirectory();
#endif
