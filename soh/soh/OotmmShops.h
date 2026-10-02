#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct EnGirlA;
struct Player;
struct PlayState;

void OotmmShops_SetupItem(struct EnGirlA* item, struct PlayState* play);
int OotmmShops_IsShuffled(const struct EnGirlA* item);
int OotmmShops_Draw(struct EnGirlA* item, struct PlayState* play);
int OotmmShops_Report(struct EnGirlA* item);
int OotmmShops_ContinuePresentation(struct EnGirlA* item, struct PlayState* play, struct Player* player);

#ifdef __cplusplus
}

void OotmmShops_Init();
#endif
