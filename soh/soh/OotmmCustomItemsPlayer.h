#pragma once


#ifdef __cplusplus
extern "C" {
#endif

void OotmmCustomItems_SetTunicEnvColor(u8 r, u8 g, u8 b);
Gfx* OotmmCustomItems_EquipmentDList(PlayState* play, Gfx* base, const char* const* parts, int count);
Gfx* OotmmCustomItems_GreatFairySwordHand(PlayState* play, Gfx* handDList);
void OotmmCustomItems_DrawMaskWithMatrix(PlayState* play, uint8_t mask, Mtx* matrix);
int OotmmCustomItems_ActorIgnoresPlayer(Actor* actor, Player* player);
int OotmmCustomItems_TryBlastMask(PlayState* play, Player* player);
void OotmmCustomItems_TickTimers(void);
const char* OotmmCustomItems_KamaroDanceAnim(void);
void* OotmmCustomItems_BButtonIcon(uint8_t item);
const char* OotmmCustomItems_BButtonLabel(void);
uint8_t OotmmCustomItems_BButtonShade(void);
void OotmmCustomItems_CaptureMaskMatrix(PlayState* play, Player* player);
void OotmmCustomItems_DrawWornMask(PlayState* play, Player* player);
int OotmmCustomItems_ExplosiveAmmo(int explosiveType);

int OotmmCustomItems_KegFuse(void);
void OotmmCustomItems_SetKegFuse(int fuse);
Actor* OotmmCustomItems_SpawnExplosive(PlayState* play, Player* player, int explosiveType, s16 actorId);

#ifdef __cplusplus
}
#endif
