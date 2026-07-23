#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <libultraship/bridge/OotmmGameState.h>

#include <string>

extern "C" {
#endif

int32_t OotmmSession_IsActive(void);
int32_t OotmmSession_TryBootDirectly(void* gameState);
void OotmmSession_InitializeNewSave(void);
void OotmmSession_NotePlayerExitTransition(void);
void OotmmSession_PrepareGrottoReturn(void);

#ifdef __cplusplus
}

void OotmmSession_Init();
const Ship::OotmmGameState& OotmmSession_GetState();
std::string OotmmSession_GetSaveSubdirectory();
#endif
