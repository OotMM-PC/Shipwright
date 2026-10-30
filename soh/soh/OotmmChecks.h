#pragma once

struct Actor;
struct PlayState;
struct Player;

#ifdef __cplusplus
#include <libultraship/bridge/GameIpcSession.h>
#include <libultraship/bridge/OotmmGameState.h>

extern "C" {
#endif

int OotmmChecks_BossContainerDone(struct PlayState* play, int vanillaDone);
int OotmmChecks_BossContainerIsShuffled(struct PlayState* play);
int OotmmChecks_TryCollectBossContainer(struct Actor* actor, struct PlayState* play);
int OotmmChecks_DrawBossContainer(struct Actor* actor, struct PlayState* play);
int OotmmChecks_DrawToken(struct Actor* actor, struct PlayState* play);
int OotmmChecks_CollectibleDone(struct PlayState* play, int flag, int vanillaDone);
int OotmmChecks_DrawDivingRupee(struct Actor* actor, struct PlayState* play, int colorIdx);
int OotmmChecks_CollectDivingRupee(struct PlayState* play, int colorIdx);
int OotmmChecks_SilverRupeeIsShuffled(int index);
void* OotmmChecks_SkulltulaDL(struct Actor* skulltula, const char* part, void* vanilla);
void* OotmmChecks_CowRingDL(struct Actor* cow, void* vanilla);
int OotmmChecks_SilverRupeeTaken(int index);
int OotmmChecks_DrawSilverRupee(struct Actor* actor, struct PlayState* play, int index);
void OotmmChecks_CollectSilverRupee(int index);
int OotmmChecks_SpawnCollectibleDrop(struct PlayState* play, int flag, float x, float y, float z,
                                     struct Actor** drop);

#ifdef __cplusplus
}

void OotmmChecks_Init();
void OotmmChecks_OpenChestSilently(struct PlayState* play, struct Player* player);
#endif
