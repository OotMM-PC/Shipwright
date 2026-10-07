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
#if !defined(NDEBUG) || defined(OOTMM_RUNTIME_TEST)
struct OotmmChestPresentationDebugState {
    int Attempts = 0;
    int Handoffs = 0;
    int ActorId = -1;
    float ActorScale = 0.0f;
    bool MatchedQueue = false;
    bool Large = false;
    bool Delivered = false;
};
OotmmChestPresentationDebugState OotmmItemPresentation_DebugChestState();
int OotmmItemPresentation_DebugLastFanfare();
#endif
std::string OotmmItemPresentation_GameName(const std::string& game);

extern "C" {
#endif

int OotmmItemPresentation_IsPending(void);
int OotmmItemPresentation_PlayFanfare(int isShort);

#ifdef __cplusplus
}
#endif
