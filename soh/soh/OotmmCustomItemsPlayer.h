#pragma once

// PlayState, Actor and Player are anonymous typedefs, so include this after z64.h.

#ifdef __cplusplus
extern "C" {
#endif

void OotmmCustomItems_SetTunicEnvColor(u8 r, u8 g, u8 b);
Gfx* OotmmCustomItems_GreatFairySwordHand(PlayState* play, Gfx* handDList);
/// Draws a worn cross-game mask at an explicit head-limb matrix, for remote-player puppets.
void OotmmCustomItems_DrawMaskWithMatrix(PlayState* play, uint8_t mask, Mtx* matrix);
int OotmmCustomItems_ActorIgnoresPlayer(Actor* actor, Player* player);
/// Returns 1 when the worn Blast Mask detonated and consumed the B press.
int OotmmCustomItems_TryBlastMask(PlayState* play, Player* player);
/// Ticks the Blast Mask cooldown; call once per player update.
void OotmmCustomItems_TickTimers(void);
const char* OotmmCustomItems_KamaroDanceAnim(void);
void* OotmmCustomItems_BButtonIcon(uint8_t item);
/// Do-action label the B button should show instead of its icon, or NULL.
const char* OotmmCustomItems_BButtonLabel(void);
uint8_t OotmmCustomItems_BButtonShade(void);
/// Call from the head limb; the mask is drawn later, after the skeleton.
void OotmmCustomItems_CaptureMaskMatrix(PlayState* play, Player* player);
void OotmmCustomItems_DrawWornMask(PlayState* play, Player* player);
/// Ammo available for a custom explosive, standing in for the AMMO() macro.
int OotmmCustomItems_ExplosiveAmmo(int explosiveType);

void OotmmCustomItems_MarkExplosive(Actor* spawned, int explosiveType);
float OotmmCustomItems_ExplosiveScale(Actor* actor);
/// Returns 1 when it drew the keg in place of the bomb model.
int OotmmCustomItems_DrawPowderKeg(PlayState* play, Actor* actor);

#ifdef __cplusplus
}
#endif
