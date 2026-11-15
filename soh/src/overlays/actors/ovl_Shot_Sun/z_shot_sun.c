/*
 * File: z_shot_sun.c
 * Overlay: ovl_Shot_Sun
 * Description: Lake Hylia Sun hitbox and Song of Storms Fairy spawner
 */

#include "z_shot_sun.h"
#include "soh/OotmmXflags.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "overlays/actors/ovl_En_Okarina_Tag/z_en_okarina_tag.h"
#include "soh/OotmmSession.h"
#include "scenes/overworld/spot06/spot06_scene.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "vt.h"

#define FLAGS (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY)

void ShotSun_Init(Actor* thisx, PlayState* play);
void ShotSun_Destroy(Actor* thisx, PlayState* play);
void ShotSun_Update(Actor* thisx, PlayState* play);

void ShotSun_SpawnFairy(ShotSun* this, PlayState* play);
void ShotSun_TriggerFairy(ShotSun* this, PlayState* play);
void func_80BADF0C(ShotSun* this, PlayState* play);
void ShotSun_UpdateHyliaSun(ShotSun* this, PlayState* play);
void ShotSun_DrawSongPad(Actor* thisx, PlayState* play);

const ActorInit Shot_Sun_InitVars = {
    ACTOR_SHOT_SUN,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(ShotSun),
    (ActorFunc)ShotSun_Init,
    (ActorFunc)ShotSun_Destroy,
    (ActorFunc)ShotSun_Update,
    NULL,
    NULL,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0x00000000, 0x00, 0x00 },
        { 0x00000020, 0x00, 0x00 },
        TOUCH_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 30, 60, 0, { 0, 0, 0 } },
};

static f32 ShotSun_SongPadHeight(PlayState* play, Vec3f* centre) {
    static const f32 sCorners[][2] = {
        { 0.0f, 0.0f }, { 30.0f, 30.0f }, { 30.0f, -30.0f }, { -30.0f, 30.0f }, { -30.0f, -30.0f },
    };
    CollisionPoly* floorPoly;
    WaterBox* waterBox;
    f32 height = BGCHECK_Y_MIN;
    f32 surface;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sCorners); i++) {
        Vec3f probe = { centre->x + sCorners[i][0], centre->y + 50.0f, centre->z + sCorners[i][1] };
        f32 floor = BgCheck_EntityRaycastFloor1(&play->colCtx, &floorPoly, &probe);

        if (floor <= BGCHECK_Y_MIN) {
            continue;
        }
        if (height <= BGCHECK_Y_MIN || (floor > height && floor < height + 25.0f)) {
            height = floor;
        }
    }
    if (height <= BGCHECK_Y_MIN) {
        height = centre->y;
    }
    surface = height;
    if (WaterBox_GetSurface1(play, &play->colCtx, centre->x, centre->z, &surface, &waterBox) && surface > height) {
        height = surface + 1.0f;
    }
    return height;
}

void ShotSun_Init(Actor* thisx, PlayState* play) {
    ShotSun* this = (ShotSun*)thisx;
    s32 params;

    // "Ocarina secret occurrence"
    osSyncPrintf("%d ---- オカリナの秘密発生!!!!!!!!!!!!!\n", this->actor.params);
    params = this->actor.params & 0xFF;
    if (params == 0x40 || params == 0x41) {
        this->unk_1A4 = 0;
        this->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        this->actor.flags |= ACTOR_FLAG_UPDATE_DURING_OCARINA;
        this->actionFunc = func_80BADF0C;
        this->actor.flags |= ACTOR_FLAG_LOCK_ON_DISABLED;
        if (OotmmSession_IsActive()) {
            this->actor.floorHeight = ShotSun_SongPadHeight(play, &this->actor.world.pos);
            this->actor.draw = ShotSun_DrawSongPad;
        }
    } else {
        Collider_InitCylinder(play, &this->collider);
        Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
        this->actionFunc = ShotSun_UpdateHyliaSun;
        this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    }
}

void ShotSun_Destroy(Actor* thisx, PlayState* play) {
    ShotSun* this = (ShotSun*)thisx;
    s32 params = this->actor.params & 0xFF;

    if (params != 0x40 && params != 0x41) {
        Collider_DestroyCylinder(play, &this->collider);
    }
}

void ShotSun_SpawnFairy(ShotSun* this, PlayState* play) {
    s32 params = this->actor.params & 0xFF;
    s32 fairyType;

    if (this->timer > 0) {
        this->timer--;
    } else {
        switch (params) {
            case 0x40:
                fairyType = FAIRY_HEAL_BIG;
                break;
            case 0x41:
                fairyType = FAIRY_HEAL_BIG;
                break;
        }

        //! @bug fairyType may be uninitialized
        if (GameInteractor_Should(VB_SPAWN_SONG_FAIRY, true, this)) {
            OotmmXflags_PrepareSpawn(&this->actor, 0);
            Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ELF, this->actor.home.pos.x, this->actor.home.pos.y,
                        this->actor.home.pos.z, 0, 0, 0, fairyType);
            OotmmXflags_ClearSpawn();
        }

        Actor_Kill(&this->actor);
    }
}

void ShotSun_TriggerFairy(ShotSun* this, PlayState* play) {
    if ((func_8005B198() == this->actor.category) || (this->timer != 0)) {
        this->actionFunc = ShotSun_SpawnFairy;
        this->timer = 50;

        Actor_Spawn(&play->actorCtx, play, ACTOR_DEMO_KANKYO, this->actor.home.pos.x, this->actor.home.pos.y,
                    this->actor.home.pos.z, 0, 0, 0, 0x11);

        Sfx_PlaySfxAtPos(&this->actor.projectedPos, NA_SE_EV_TRE_BOX_APPEAR);
    }
}

void func_80BADF0C(ShotSun* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s32 pad;
    s32 params = this->actor.params & 0xFF;

    if (Math3D_Vec3fDistSq(&this->actor.world.pos, &player->actor.world.pos) > 22500.0f) {
        this->unk_1A4 = 0;
    } else {
        if (this->unk_1A4 == 0) {
            if (!(player->stateFlags2 & PLAYER_STATE2_ATTEMPT_PLAY_FOR_ACTOR)) {
                player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
                return;
            } else {
                this->unk_1A4 = 1;
            }
        }
        if (this->unk_1A4 == 1) {
            func_8010BD58(play, OCARINA_ACTION_FREE_PLAY);
            this->unk_1A4 = 2;
        } else if (this->unk_1A4 == 2 && play->msgCtx.ocarinaMode == OCARINA_MODE_04) {
            if ((params == 0x40 && play->msgCtx.lastPlayedSong == OCARINA_SONG_SUNS) ||
                (params == 0x41 && play->msgCtx.lastPlayedSong == OCARINA_SONG_STORMS)) {
                this->actionFunc = ShotSun_TriggerFairy;
                OnePointCutscene_Attention(play, &this->actor);
                this->timer = 0;
            } else {
                this->unk_1A4 = 0;
            }
            this->unk_1A4 = 0;
        }
    }
}

void ShotSun_UpdateHyliaSun(ShotSun* this, PlayState* play) {
    Vec3s cylinderPos;
    Player* player = GET_PLAYER(play);
    EnItem00* collectible;
    s32 pad;
    Vec3f spawnPos;

    if (this->collider.base.acFlags & AC_HIT) {
        Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        osSyncPrintf(VT_FGCOL(CYAN) "SHOT_SUN HIT!!!!!!!\n" VT_RST);
        if (GameInteractor_Should(VB_SPAWN_FIRE_ARROW, INV_CONTENT(ITEM_ARROW_FIRE) == ITEM_NONE)) {
            Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_ETCETERA, 700.0f, -800.0f, 7261.0f, 0, 0, 0, 7);
            if (GameInteractor_Should(VB_PLAY_FIRE_ARROW_CS, true)) {
                play->csCtx.segment = SEGMENTED_TO_VIRTUAL(gLakeHyliaFireArrowsCS);
                gSaveContext.cutsceneTrigger = 1;
            }
        } else {
            spawnPos.x = 700.0f;
            spawnPos.y = -800.0f;
            spawnPos.z = 7261.0f;

            collectible = Item_DropCollectible(play, &spawnPos, ITEM00_MAGIC_LARGE);
            if (collectible != NULL) {
                collectible->unk_15A = 6000;
                collectible->actor.speedXZ = 0.0f;
            }
        }
        Actor_Kill(&this->actor);
    } else {
        if (!(this->actor.xzDistToPlayer > 120.0f) && gSaveContext.dayTime >= 0x4555 && gSaveContext.dayTime < 0x5000) {
            cylinderPos.x = player->bodyPartsPos[7].x + play->envCtx.sunPos.x * (1.0f / 6.0f);
            cylinderPos.y = player->bodyPartsPos[7].y - 30.0f + play->envCtx.sunPos.y * (1.0f / 6.0f);
            cylinderPos.z = player->bodyPartsPos[7].z + play->envCtx.sunPos.z * (1.0f / 6.0f);

            this->hitboxPos = cylinderPos;

            Collider_SetCylinderPosition(&this->collider, &cylinderPos);
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
        }
    }
}

void ShotSun_DrawSongPad(Actor* thisx, PlayState* play) {
    Vec3f pos = { thisx->world.pos.x, thisx->floorHeight + 0.1f, thisx->world.pos.z };

    if (!OotmmXflags_IsShuffled(thisx)) {
        return;
    }
    EnOkarinaTag_DrawSongPad(play, &pos, 0.0f, 1.2f, false,
                             (thisx->params & 0xFF) == 0x40 ? OOTMM_SONG_TAG_SUN : OOTMM_SONG_TAG_STORMS);
}

void ShotSun_Update(Actor* thisx, PlayState* play) {
    ShotSun* this = (ShotSun*)thisx;

    this->actionFunc(this, play);
}
