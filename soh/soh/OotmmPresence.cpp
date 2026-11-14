#include "soh/OotmmPresence.h"
#include "soh/OotmmSession.h"
#include "soh/OotmmIpc.h"
#include "soh/OotmmCustomItems.h"
#include "soh/OotmmCustomItemsPlayer.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/nametag.h"

#include <libultraship/bridge/OotmmPresence.h>
#include <ship/Context.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_link_child/object_link_child.h"

extern PlayState* gPlayState;
extern FlexSkeletonHeader* gPlayerSkelHeaders[2];
extern void* sEyeTextures[2][8];
extern void* sMouthTextures[2][4];

Gfx* ResourceMgr_LoadGfxByName(const char* path);
uint8_t ResourceMgr_FileExists(const char* resName);
void ResourceMgr_UnregisterSkeleton(SkelAnime* skelAnime);

void Player_DrawImpl(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount, s32 lod, s32 tunic,
                     s32 boots, s32 face, OverrideLimbDrawOpa overrideLimbDraw, PostLimbDrawOpa postLimbDraw,
                     void* data);
}

s16 gOotmmPuppetId = -1;
s32 gOotmmPvpHitsSent = 0;
s32 gOotmmPvpHitsReceived = 0;
s32 gOotmmPvpHitsApplied = 0;
const char* gOotmmEquipDlCapture[4] = { NULL, NULL, NULL, NULL };
s32 gOotmmEquipTypeCapture[2] = { -1, -1 };

namespace {

int32_t gPuppetDrawAge = -1;
int32_t gPuppetPlayerId = 0;
int32_t gPuppetMoveFlags = 0;
int32_t gPuppetItemAction = -1;
int32_t gPuppetCustomMask = 0;
Mtx* gPuppetMaskMatrix = nullptr;

const char* PuppetNamespaced(char* buf, size_t cap, int32_t playerId, const char* canonical) {
    if (playerId == 0 || canonical == nullptr || std::strncmp(canonical, "objects/", 8) != 0) {
        return nullptr;
    }
    std::snprintf(buf, cap, "__OTR__p%02xobjs/%s", playerId & 0xFF, canonical + 8);
    return ResourceMgr_FileExists(buf + 7) ? buf : nullptr;
}

Gfx* PuppetLoadGfx(const char* canonicalPath) {
    if (canonicalPath != nullptr && std::strncmp(canonicalPath, "__OTR__", 7) == 0) {
        canonicalPath += 7;
    }
    if (canonicalPath == nullptr) {
        return nullptr;
    }
    if (std::strncmp(canonicalPath, "alt/", 4) == 0) {
        canonicalPath += 4;
    }
    char ns[128];
    if (PuppetNamespaced(ns, sizeof(ns), gPuppetPlayerId, canonicalPath) != nullptr) {
        return ResourceMgr_LoadGfxByName(ns + 7);
    }
    return ResourceMgr_FileExists(canonicalPath) ? ResourceMgr_LoadGfxByName(canonicalPath) : nullptr;
}

} // namespace

extern "C" void* OotmmPuppet_EyeTexture(s32 eyeIndex) {
    const int32_t age = gPuppetDrawAge >= 0 ? (gPuppetDrawAge & 1) : (gSaveContext.linkAge & 1);
    void* fallback = sEyeTextures[age][eyeIndex & 7];
    const char* live = static_cast<const char*>(fallback);
    if (gPuppetDrawAge < 0 || live == nullptr || std::strncmp(live, "__OTR__", 7) != 0) {
        return fallback;
    }
    static char sPath[128];
    return PuppetNamespaced(sPath, sizeof(sPath), gPuppetPlayerId, live + 7) != nullptr ? sPath : fallback;
}

extern "C" void* OotmmPuppet_MouthTexture(s32 mouthIndex) {
    const int32_t age = gPuppetDrawAge >= 0 ? (gPuppetDrawAge & 1) : (gSaveContext.linkAge & 1);
    void* fallback = sMouthTextures[age][mouthIndex & 3];
    const char* live = static_cast<const char*>(fallback);
    if (gPuppetDrawAge < 0 || live == nullptr || std::strncmp(live, "__OTR__", 7) != 0) {
        return fallback;
    }
    static char sPath[128];
    return PuppetNamespaced(sPath, sizeof(sPath), gPuppetPlayerId, live + 7) != nullptr ? sPath : fallback;
}

extern "C" void OotmmPuppet_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, void* data) {
    (void)dList;
    (void)rot;
    (void)data;
    if (limbIndex == PLAYER_LIMB_HEAD && gPuppetCustomMask != 0) {
        gPuppetMaskMatrix = MATRIX_NEWMTX(play->state.gfxCtx);
    }
}

extern "C" s32 OotmmPuppet_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                            void* data) {
    (void)play;
    (void)rot;
    if (limbIndex == PLAYER_LIMB_ROOT && gPuppetDrawAge == 1) {
        const int32_t mf = gPuppetMoveFlags;
        if (!(mf & 4) || (mf & 1)) {
            pos->x *= 0.64f;
            pos->z *= 0.64f;
        }
        if (!(mf & 4) || (mf & 2)) {
            pos->y *= 0.64f;
        }
    }
    OotmmPuppetActor* puppet = (OotmmPuppetActor*)data;
    if (puppet != NULL && dList != NULL) {
        const char* name = NULL;
        if (limbIndex == PLAYER_LIMB_L_HAND) {
            name = puppet->dlLeftHand;
        } else if (limbIndex == PLAYER_LIMB_R_HAND) {
            name = puppet->dlRightHand;
        } else if (limbIndex == PLAYER_LIMB_SHEATH) {
            name = puppet->dlSheath;
        } else if (limbIndex == PLAYER_LIMB_WAIST) {
            name = puppet->dlWaist;
        }
        if (name != NULL && name[0] == '-') {
            *dList = NULL;
        } else if (name != NULL && name[0] != '\0') {
            if (Gfx* equipped = PuppetLoadGfx(name); equipped != NULL) {
                *dList = equipped;
            }
        }
        if (limbIndex == PLAYER_LIMB_L_HAND && gPuppetItemAction == PLAYER_IA_SWORD_OOTMM_GREAT_FAIRY) {
            if (Gfx* fist = PuppetLoadGfx(reinterpret_cast<const char*>(gPlayerLeftHandClosedDLs[gPuppetDrawAge & 1]));
                fist != NULL) {
                *dList = OotmmCustomItems_GreatFairySwordHand(play, fist);
            }
        }
    }
    return false;
}

namespace {

struct PuppetSlot {
    Actor* actor = nullptr;
    int staleTicks = 0;
    uint32_t lastSeq = 0;
    std::string taggedName;
};
std::unordered_map<uint16_t, PuppetSlot> gPuppets;
uint32_t gLocalSeq = 0;
bool gPvpEnabled = false;

struct PendingPvpHit {
    uint16_t targetPlayer;
    int damage;
    int16_t yaw;
};
std::vector<PendingPvpHit> gPendingPvpHits;

void PuppetInitSkeleton(OotmmPuppetActor* puppet, PlayState* play, u8 age) {
    ResourceMgr_UnregisterSkeleton(&puppet->skelAnime);
    FlexSkeletonHeader* skeleton = gPlayerSkelHeaders[age & 1];
    const char* live = reinterpret_cast<const char*>(skeleton);
    puppet->skelPath[0] = '\0';
    if (live != nullptr && std::strncmp(live, "__OTR__", 7) == 0 &&
        PuppetNamespaced(puppet->skelPath, sizeof(puppet->skelPath), puppet->playerId, live + 7) != nullptr &&
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(puppet->skelPath + 7) != nullptr) {
        skeleton = reinterpret_cast<FlexSkeletonHeader*>(puppet->skelPath);
    }
    SkelAnime_InitLink(play, &puppet->skelAnime, skeleton,
                       (LinkAnimationHeader*)gPlayerAnim_link_normal_wait, 9, puppet->jointTable, puppet->morphTable,
                       PLAYER_LIMB_MAX);
    puppet->age = age & 1;
}

int PresencePlace(PlayState* play) {
    if (play->sceneNum != SCENE_GROTTOS && play->sceneNum != SCENE_FAIRYS_FOUNTAIN) {
        return 0;
    }
    const RespawnData& grotto = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    const int parent =
        grotto.entranceIndex >= 0 && grotto.entranceIndex < ENTR_MAX ? gEntranceTable[grotto.entranceIndex].scene : 0xFF;
    return 0x10000 | ((parent & 0xFF) << 8) | static_cast<u8>(grotto.data);
}

bool ActorStillSpawned(PlayState* play, const Actor* target) {
    for (const auto& list : play->actorCtx.actorLists) {
        for (const Actor* actor = list.head; actor != nullptr; actor = actor->next) {
            if (actor == target) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

static ColliderCylinderInit sPuppetCylinderInit = {
    {
        COLTYPE_HIT5,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK1,
        { 0x00000000, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 20, 60, 0, { 0, 0, 0 } },
};

extern "C" void OotmmPuppet_Init(Actor* thisx, PlayState* play) {
    OotmmPuppetActor* puppet = (OotmmPuppetActor*)thisx;
    Actor_SetScale(thisx, 0.01f);
    thisx->targetMode = 0;
    puppet->hasPose = false;
    PuppetInitSkeleton(puppet, play, static_cast<u8>(thisx->params & 1));
    puppet->targetPos = thisx->world.pos;
    puppet->targetYaw = thisx->shape.rot.y;
    puppet->dlLeftHand[0] = puppet->dlRightHand[0] = puppet->dlSheath[0] = puppet->dlWaist[0] = '\0';
    puppet->leftHandType = -1;
    puppet->rightHandType = -1;
    puppet->tunic = puppet->boots = puppet->strength = puppet->mask = 0;
    puppet->pvpCooldown = 0;
    puppet->pvpLastProjectile = nullptr;
    Collider_InitCylinder(play, &puppet->collider);
    Collider_SetCylinder(play, &puppet->collider, thisx, &sPuppetCylinderInit);
}

extern "C" void OotmmPuppet_Destroy(Actor* thisx, PlayState* play) {
    OotmmPuppetActor* puppet = (OotmmPuppetActor*)thisx;
    ResourceMgr_UnregisterSkeleton(&puppet->skelAnime);
    Collider_DestroyCylinder(play, &puppet->collider);
}

extern "C" void OotmmPuppet_Update(Actor* thisx, PlayState* play) {
    OotmmPuppetActor* puppet = (OotmmPuppetActor*)thisx;
    if (!puppet->hasPose) {
        return;
    }
    const float dx = puppet->targetPos.x - thisx->world.pos.x;
    const float dy = puppet->targetPos.y - thisx->world.pos.y;
    const float dz = puppet->targetPos.z - thisx->world.pos.z;
    if (dx * dx + dy * dy + dz * dz > 90000.0f) {
        thisx->world.pos = puppet->targetPos;
    } else {
        thisx->world.pos.x += dx * 0.4f;
        thisx->world.pos.y += dy * 0.4f;
        thisx->world.pos.z += dz * 0.4f;
    }
    Math_ScaledStepToS(&thisx->shape.rot.y, puppet->targetYaw, 0x1800);
    thisx->world.rot.y = thisx->shape.rot.y;
    std::memcpy(puppet->skelAnime.jointTable, puppet->netJoints, sizeof(puppet->netJoints));

    if (gPvpEnabled) {
        if (puppet->pvpCooldown > 0) {
            puppet->pvpCooldown--;
        }
        if (puppet->pvpLastProjectile != nullptr && !ActorStillSpawned(play, puppet->pvpLastProjectile)) {
            puppet->pvpLastProjectile = nullptr;
        }
        Actor* source = puppet->collider.base.ac;
        const bool projectile = source != nullptr && source != &GET_PLAYER(play)->actor;
        if ((puppet->collider.base.acFlags & AC_HIT) && puppet->pvpCooldown == 0 &&
            !(projectile && source == puppet->pvpLastProjectile)) {
            puppet->collider.base.acFlags &= ~AC_HIT;
            if (projectile) {
                puppet->pvpLastProjectile = source;
            }
            puppet->pvpCooldown = 8;
            int damage = 4;
            if (puppet->collider.info.acHitInfo != NULL && puppet->collider.info.acHitInfo->toucher.damage > 0) {
                damage = puppet->collider.info.acHitInfo->toucher.damage;
            }
            Player* attacker = GET_PLAYER(play);
            const int16_t yaw = attacker != NULL ? Math_Vec3f_Yaw(&attacker->actor.world.pos, &thisx->world.pos)
                                                 : thisx->yawTowardsPlayer;
            gPendingPvpHits.push_back(PendingPvpHit{ static_cast<uint16_t>(puppet->playerId), damage, yaw });
        }
        puppet->collider.base.acFlags &= ~AC_HIT;
        Collider_UpdateCylinder(thisx, &puppet->collider);
        CollisionCheck_SetAC(play, &play->colChkCtx, &puppet->collider.base);
        CollisionCheck_SetOC(play, &play->colChkCtx, &puppet->collider.base);
    }
}

extern "C" void OotmmPuppet_Draw(Actor* thisx, PlayState* play) {
    OotmmPuppetActor* puppet = (OotmmPuppetActor*)thisx;
    if (!puppet->hasPose || puppet->skelAnime.skeleton == NULL) {
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gPuppetDrawAge = puppet->age;
    gPuppetPlayerId = puppet->playerId;
    gPuppetMoveFlags = puppet->moveFlags;
    gPuppetItemAction = puppet->itemAction;
    gPuppetCustomMask = puppet->customMask;
    gPuppetMaskMatrix = nullptr;
    Player_DrawImpl(play, puppet->skelAnime.skeleton, puppet->skelAnime.jointTable, puppet->skelAnime.dListCount, 0,
                    puppet->tunic, puppet->boots, 0, OotmmPuppet_OverrideLimbDraw, OotmmPuppet_PostLimbDraw, puppet);
    if (gPuppetCustomMask != 0) {
        OotmmCustomItems_DrawMaskWithMatrix(play, static_cast<uint8_t>(gPuppetCustomMask), gPuppetMaskMatrix);
    }
    if (puppet->customMask == 0 && puppet->mask >= 1 && puppet->mask < PLAYER_MASK_MAX) {
        static const char* kChildMaskDls[PLAYER_MASK_MAX - 1] = {
            gLinkChildKeatonMaskDL, gLinkChildSkullMaskDL, gLinkChildSpookyMaskDL, gLinkChildBunnyHoodDL,
            gLinkChildGoronMaskDL,  gLinkChildZoraMaskDL,  gLinkChildGerudoMaskDL, gLinkChildMaskOfTruthDL,
        };
        if (puppet->mask == PLAYER_MASK_BUNNY) {
            Mtx* earMtx = (Mtx*)Graph_Alloc(play->state.gfxCtx, 2 * sizeof(Mtx));
            Vec3s earRot;
            gSPSegment(POLY_OPA_DISP++, 0x0B, reinterpret_cast<uintptr_t>(earMtx));
            earRot.x = 0x3E2;
            earRot.y = 0xDBE;
            earRot.z = -0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, -240.0f, &earRot);
            Matrix_ToMtx(earMtx++, const_cast<char*>(__FILE__), __LINE__);
            earRot.x = -0x3E2;
            earRot.y = -0xDBE;
            earRot.z = -0x348A;
            Matrix_SetTranslateRotateYXZ(97.0f, -1203.0f, 240.0f, &earRot);
            Matrix_ToMtx(earMtx, const_cast<char*>(__FILE__), __LINE__);
        }
        if (Gfx* maskDl = PuppetLoadGfx(kChildMaskDls[puppet->mask - 1]); maskDl != NULL) {
            gSPDisplayList(POLY_OPA_DISP++, maskDl);
        }
    }
    gPuppetDrawAge = -1;
    gPuppetPlayerId = 0;
    gPuppetItemAction = -1;
    gPuppetCustomMask = 0;
    gPuppetMaskMatrix = nullptr;
    CLOSE_DISPS(play->state.gfxCtx);
}

namespace {

void ApplyPoseToPuppet(PuppetSlot& slot, PlayState* play, const Ship::OotmmPlayerPose& pose) {
    Actor* actor = slot.actor;
    OotmmPuppetActor* puppet = (OotmmPuppetActor*)actor;
    const u8 age = static_cast<u8>(pose.Form & 1);
    const u8 playerId = static_cast<u8>(pose.PlayerId & 0xFF);
    const bool ownerChanged = playerId != puppet->playerId;
    puppet->playerId = playerId;
    puppet->moveFlags = static_cast<u8>(pose.MoveFlags & 0xFF);
    if (age != puppet->age || ownerChanged) {
        PuppetInitSkeleton(puppet, play, age);
    }
    std::snprintf(puppet->dlLeftHand, sizeof(puppet->dlLeftHand), "%s", pose.DlLeftHand.c_str());
    std::snprintf(puppet->dlRightHand, sizeof(puppet->dlRightHand), "%s", pose.DlRightHand.c_str());
    std::snprintf(puppet->dlSheath, sizeof(puppet->dlSheath), "%s", pose.DlSheath.c_str());
    std::snprintf(puppet->dlWaist, sizeof(puppet->dlWaist), "%s", pose.DlWaist.c_str());
    puppet->leftHandType = static_cast<s16>(pose.LeftHandType);
    puppet->rightHandType = static_cast<s16>(pose.RightHandType);
    puppet->tunic = static_cast<u8>(pose.Tunic & 0xFF);
    puppet->boots = static_cast<u8>(pose.Boots & 0xFF);
    puppet->strength = static_cast<u8>(pose.Strength & 0xFF);
    puppet->mask = static_cast<u8>(pose.Mask & 0xFF);
    puppet->itemAction = pose.ItemAction;
    puppet->customMask = pose.CustomMask;
    const size_t count = std::min(pose.JointTable.size() / 3, static_cast<size_t>(OOTMM_PUPPET_LIMB_BUF));
    for (size_t i = 0; i < count; ++i) {
        puppet->netJoints[i].x = pose.JointTable[i * 3 + 0];
        puppet->netJoints[i].y = pose.JointTable[i * 3 + 1];
        puppet->netJoints[i].z = pose.JointTable[i * 3 + 2];
    }
    puppet->targetPos.x = pose.Pos[0];
    puppet->targetPos.y = pose.Pos[1];
    puppet->targetPos.z = pose.Pos[2];
    puppet->targetYaw = pose.Yaw;
    actor->shape.yOffset = pose.YOffset;
    if (!puppet->hasPose) {
        actor->world.pos = puppet->targetPos;
        actor->shape.rot.y = puppet->targetYaw;
    }
    puppet->hasPose = true;
}

std::string CanonicalDl(const char* name) {
    if (name == nullptr) {
        return {};
    }
    if (name[0] == '-') {
        return "-";
    }
    if (std::strncmp(name, "__OTR__", 7) == 0) {
        name += 7;
    }
    if (std::strncmp(name, "alt/", 4) == 0) {
        name += 4;
    }
    return name;
}

void PresenceTick() {
    if (gPlayState == nullptr || !OotmmSession_IsActive()) {
        return;
    }
    if (!OotmmIpc_PresenceActive()) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return;
    }
    const uint16_t localPlayer = static_cast<uint16_t>(OotmmSession_GetState().GetPlayerId());
    for (auto& [id, slot] : gPuppets) {
        if (slot.actor != nullptr && !ActorStillSpawned(gPlayState, slot.actor)) {
            slot.actor = nullptr;
            slot.taggedName.clear();
        }
    }

    Ship::OotmmPlayerPose pose;
    pose.PlayerId = localPlayer;
    pose.Game = "oot";
    pose.SceneId = gPlayState->sceneNum;
    pose.RoomId = gPlayState->roomCtx.curRoom.num;
    pose.Place = PresencePlace(gPlayState);
    pose.Pos[0] = player->actor.world.pos.x;
    pose.Pos[1] = player->actor.world.pos.y;
    pose.Pos[2] = player->actor.world.pos.z;
    pose.Yaw = player->actor.shape.rot.y;
    pose.YOffset = player->actor.shape.yOffset;
    pose.MoveFlags = player->skelAnime.movementFlags;
    pose.Form = gSaveContext.linkAge;
    pose.Seq = ++gLocalSeq;
    pose.DlLeftHand = CanonicalDl(gOotmmEquipDlCapture[0]);
    pose.DlRightHand = CanonicalDl(gOotmmEquipDlCapture[1]);
    pose.DlSheath = CanonicalDl(gOotmmEquipDlCapture[2]);
    pose.DlWaist = CanonicalDl(gOotmmEquipDlCapture[3]);
    pose.LeftHandType = gOotmmEquipTypeCapture[0];
    pose.RightHandType = gOotmmEquipTypeCapture[1];
    pose.Tunic = player->currentTunic;
    pose.Boots = player->currentBoots;
    pose.Strength = CUR_UPG_VALUE(UPG_STRENGTH);
    pose.Mask = player->currentMask;
    pose.ItemAction = player->heldItemAction;
    pose.CustomMask =
        OotmmCustomItems_MaskDList(player->currentMask) != nullptr ? player->currentMask : 0;
    if (player->skelAnime.jointTable != nullptr) {
        pose.JointTable.resize(OOTMM_PUPPET_LIMB_BUF * 3);
        for (int i = 0; i < OOTMM_PUPPET_LIMB_BUF && i < PLAYER_LIMB_MAX; ++i) {
            pose.JointTable[i * 3 + 0] = player->skelAnime.jointTable[i].x;
            pose.JointTable[i * 3 + 1] = player->skelAnime.jointTable[i].y;
            pose.JointTable[i * 3 + 2] = player->skelAnime.jointTable[i].z;
        }
    }
    OotmmIpc_SendPlayerPose(pose);

    for (auto& [id, slot] : gPuppets) {
        slot.staleTicks++;
    }
    std::vector<Ship::OotmmPlayerPose> roster;
    std::vector<Ship::OotmmPvpHit> pvpHits;
    const bool freshRoster = OotmmIpc_TakeRemotePresence(roster, pvpHits);
    gPvpEnabled = gOotmmPvpEnabled != 0;

    if (!gPendingPvpHits.empty()) {
        static uint32_t pvpHitSeq = 0;
        for (const PendingPvpHit& pending : gPendingPvpHits) {
            Ship::OotmmPvpHit hit;
            hit.SourcePlayer = localPlayer;
            hit.TargetPlayer = pending.targetPlayer;
            hit.Damage = pending.damage;
            hit.Yaw = pending.yaw;
            hit.Seq = ++pvpHitSeq;
            OotmmIpc_SendPvpHit(hit);
            gOotmmPvpHitsSent++;
        }
        gPendingPvpHits.clear();
    }

    {
        for (const Ship::OotmmPvpHit& hit : pvpHits) {
            gOotmmPvpHitsReceived++;
            if (hit.TargetPlayer != localPlayer || !gPvpEnabled) {
                continue;
            }
            Player* self = GET_PLAYER(gPlayState);
            if (self != nullptr && self->invincibilityTimer == 0 && !(self->stateFlags1 & PLAYER_STATE1_DEAD)) {
                Actor* source = &self->actor;
                if (const auto it = gPuppets.find(hit.SourcePlayer); it != gPuppets.end() && it->second.actor != nullptr) {
                    source = it->second.actor;
                }
                func_8002F6D4(gPlayState, source, 6.0f, hit.Yaw, 5.0f, static_cast<u32>(std::clamp(hit.Damage, 1, 255)));
                gOotmmPvpHitsApplied++;
            }
        }
    }

    if (!freshRoster) {
        return;
    }

    for (const Ship::OotmmPlayerPose& remote : roster) {
        if (remote.PlayerId == localPlayer || remote.Game != "oot" || remote.SceneId != gPlayState->sceneNum ||
            remote.Place != pose.Place) {
            continue;
        }
        PuppetSlot& slot = gPuppets[remote.PlayerId];
        if (slot.actor != nullptr && remote.Seq != 0 && remote.Seq == slot.lastSeq) {
            slot.staleTicks--;
        }
        if (slot.actor == nullptr && gOotmmPuppetId >= 0) {
            slot.actor = Actor_Spawn(&gPlayState->actorCtx, gPlayState, gOotmmPuppetId, remote.Pos[0], remote.Pos[1],
                                     remote.Pos[2], 0, remote.Yaw, 0, static_cast<s16>(remote.Form & 1));
            if (slot.actor != nullptr) {
                slot.actor->room = -1;
            }
        }
        if (slot.actor != nullptr) {
            ApplyPoseToPuppet(slot, gPlayState, remote);
            slot.staleTicks = 0;
            slot.lastSeq = remote.Seq;
            const std::string wanted = gOotmmShowNames != 0
                                           ? (!remote.ClientName.empty() ? remote.ClientName
                                                                         : "Player " + std::to_string(remote.PlayerId))
                                           : "";
            if (wanted != slot.taggedName) {
                NameTag_RemoveAllForActor(slot.actor);
                if (!wanted.empty()) {
                    NameTagOptions options = {};
                    options.tag = "ootmm_coop";
                    options.yOffset = 44;
                    NameTag_RegisterForActorWithOptions(slot.actor, wanted.c_str(), options);
                }
                slot.taggedName = wanted;
            }
        }
    }
    for (auto it = gPuppets.begin(); it != gPuppets.end();) {
        if (it->second.staleTicks > 60) {
            if (it->second.actor != nullptr) {
                Actor_Kill(it->second.actor);
            }
            it = gPuppets.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace

extern "C" void OotmmPresence_RefreshPuppetModel(uint16_t playerId) {
    const auto it = gPuppets.find(playerId);
    if (gPlayState == nullptr || it == gPuppets.end() || it->second.actor == nullptr ||
        !ActorStillSpawned(gPlayState, it->second.actor)) {
        return;
    }
    auto* puppet = (OotmmPuppetActor*)it->second.actor;
    PuppetInitSkeleton(puppet, gPlayState, puppet->age);
}

extern "C" void OotmmPresence_Init(void) {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() { PresenceTick(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { gPuppets.clear(); });
}
