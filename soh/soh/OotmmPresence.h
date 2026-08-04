#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <z64.h>

#define OOTMM_PUPPET_LIMB_BUF 24
#define OOTMM_PUPPET_DL_NAME_BUF 96

typedef struct OotmmPuppetActor {
    Actor actor;
    SkelAnime skelAnime;
    Vec3s jointTable[OOTMM_PUPPET_LIMB_BUF];
    Vec3s morphTable[OOTMM_PUPPET_LIMB_BUF];
    Vec3s netJoints[OOTMM_PUPPET_LIMB_BUF];
    Vec3f targetPos;
    s16 targetYaw;
    u8 age;
    u8 hasPose;
    u8 playerId;
    u8 moveFlags; // sender's skelAnime.movementFlags (conditions the root-limb draw scaling)
    // The player draw substitutes these four limbs BY NAME every frame; the puppet replays the
    // sender's choice. "" = the skeleton's limb DL, "-" = draw nothing.
    char dlLeftHand[OOTMM_PUPPET_DL_NAME_BUF];
    char dlRightHand[OOTMM_PUPPET_DL_NAME_BUF];
    char dlSheath[OOTMM_PUPPET_DL_NAME_BUF];
    char dlWaist[OOTMM_PUPPET_DL_NAME_BUF];
    s16 leftHandType;
    s16 rightHandType;
    u8 tunic;
    u8 boots;
    u8 strength;
    u8 mask;
    s32 itemAction;
    s32 customMask;
    // PvP hurtbox the local player's weapons connect with; hits are relayed to the victim.
    ColliderCylinder collider;
    u8 pvpCooldown;
} OotmmPuppetActor;

void OotmmPuppet_Init(Actor* thisx, PlayState* play);
void OotmmPuppet_Destroy(Actor* thisx, PlayState* play);
void OotmmPuppet_Update(Actor* thisx, PlayState* play);
void OotmmPuppet_Draw(Actor* thisx, PlayState* play);

void OotmmPresence_Init(void);

extern s16 gOotmmPuppetId;
// What the local player's draw chose for the four equipment limbs + hand model types this frame.
extern const char* gOotmmEquipDlCapture[4];
extern s32 gOotmmEquipTypeCapture[2];

#ifdef __cplusplus
}
#endif
