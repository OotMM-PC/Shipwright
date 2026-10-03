#pragma once

#ifdef __cplusplus
#include <libultraship/bridge/GameIpcSession.h>

#include <string>

struct PlayState;
struct Player;

void OotmmItemPresentation_Init();
void OotmmItemPresentation_Queue(Ship::GameIpcItemPresentation presentation);
bool OotmmItemPresentation_ContinueChest(PlayState* play, Player* player, const std::string& checkId);
bool OotmmItemPresentation_IsDelivering(const std::string& checkId);
bool OotmmItemPresentation_PlayerReceiving(const Player* player);
std::string OotmmItemPresentation_GameName(const std::string& game);

extern "C" {
#endif

int OotmmItemPresentation_IsPending(void);

#ifdef __cplusplus
}
#endif
