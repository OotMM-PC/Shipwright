/*
 * File: z_en_okarina_tag.c
 * Overlay: ovl_En_Okarina_Tag
 * Description: Music Staff (Ocarina) spot
 */

#include "z_en_okarina_tag.h"
#include "scenes/misc/hakaana_ouke/hakaana_ouke_scene.h"
#include "scenes/overworld/spot02/spot02_scene.h"
#include "vt.h"
#include "align_asset_macro.h"
#include "soh/OotmmSongEvents.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA)

void EnOkarinaTag_Init(Actor* thisx, PlayState* play);
void EnOkarinaTag_Destroy(Actor* thisx, PlayState* play);
void EnOkarinaTag_Update(Actor* thisx, PlayState* play);

void func_80ABEF2C(EnOkarinaTag* this, PlayState* play);
void func_80ABF708(EnOkarinaTag* this, PlayState* play);
void func_80ABF28C(EnOkarinaTag* this, PlayState* play);
void func_80ABF0CC(EnOkarinaTag* this, PlayState* play);
void func_80ABF4C8(EnOkarinaTag* this, PlayState* play);
void func_80ABF7CC(EnOkarinaTag* this, PlayState* play);

const ActorInit En_Okarina_Tag_InitVars = {
    ACTOR_EN_OKARINA_TAG,
    ACTORCAT_PROP,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(EnOkarinaTag),
    (ActorFunc)EnOkarinaTag_Init,
    (ActorFunc)EnOkarinaTag_Destroy,
    (ActorFunc)EnOkarinaTag_Update,
    NULL,
    NULL,
};

extern CutsceneData D_80ABF9D0[];
extern CutsceneData D_80ABFB40[];

static const ALIGN_ASSET(2) char sSongTagLullabyTex[] = "__OTR__objects/ootmm_song_tags/gSongTagLullabyTex";
static const ALIGN_ASSET(2) char sSongTagEponaTex[] = "__OTR__objects/ootmm_song_tags/gSongTagEponaTex";
static const ALIGN_ASSET(2) char sSongTagSariaTex[] = "__OTR__objects/ootmm_song_tags/gSongTagSariaTex";
static const ALIGN_ASSET(2) char sSongTagStormsTex[] = "__OTR__objects/ootmm_song_tags/gSongTagStormsTex";
static const ALIGN_ASSET(2) char sSongTagSunTex[] = "__OTR__objects/ootmm_song_tags/gSongTagSunTex";
static const ALIGN_ASSET(2) char sSongTagTimeTex[] = "__OTR__objects/ootmm_song_tags/gSongTagTimeTex";
static const ALIGN_ASSET(2) char sSongTagMinuetTex[] = "__OTR__objects/ootmm_song_tags/gSongTagMinuetTex";
static const ALIGN_ASSET(2) char sSongTagBoleroTex[] = "__OTR__objects/ootmm_song_tags/gSongTagBoleroTex";
static const ALIGN_ASSET(2) char sSongTagSerenadeTex[] = "__OTR__objects/ootmm_song_tags/gSongTagSerenadeTex";
static const ALIGN_ASSET(2) char sSongTagRequiemTex[] = "__OTR__objects/ootmm_song_tags/gSongTagRequiemTex";
static const ALIGN_ASSET(2) char sSongTagNocturneTex[] = "__OTR__objects/ootmm_song_tags/gSongTagNocturneTex";
static const ALIGN_ASSET(2) char sSongTagPreludeTex[] = "__OTR__objects/ootmm_song_tags/gSongTagPreludeTex";
static const ALIGN_ASSET(2) char sSongTagHealingTex[] = "__OTR__objects/ootmm_song_tags/gSongTagHealingTex";
static const ALIGN_ASSET(2) char sSongTagSoaringTex[] = "__OTR__objects/ootmm_song_tags/gSongTagSoaringTex";
static const ALIGN_ASSET(2) char sSongTagAwakeningTex[] = "__OTR__objects/ootmm_song_tags/gSongTagAwakeningTex";
static const ALIGN_ASSET(2) char sSongTagGoronTex[] = "__OTR__objects/ootmm_song_tags/gSongTagGoronTex";
static const ALIGN_ASSET(2) char sSongTagGoronHalfTex[] = "__OTR__objects/ootmm_song_tags/gSongTagGoronHalfTex";
static const ALIGN_ASSET(2) char sSongTagZoraTex[] = "__OTR__objects/ootmm_song_tags/gSongTagZoraTex";
static const ALIGN_ASSET(2) char sSongTagEmptinessTex[] = "__OTR__objects/ootmm_song_tags/gSongTagEmptinessTex";
static const ALIGN_ASSET(2) char sSongTagOrderTex[] = "__OTR__objects/ootmm_song_tags/gSongTagOrderTex";
static const ALIGN_ASSET(2) char sSongTagPadTex[] = "__OTR__objects/ootmm_song_tags/gSongTagPadTex";

static const char* sOotmmSongTagTextures[] = {
    sSongTagLullabyTex, sSongTagEponaTex,    sSongTagSariaTex,     sSongTagStormsTex, sSongTagSunTex,
    sSongTagTimeTex,    sSongTagMinuetTex,   sSongTagBoleroTex,    sSongTagSerenadeTex, sSongTagRequiemTex,
    sSongTagNocturneTex, sSongTagPreludeTex, sSongTagHealingTex,   sSongTagSoaringTex, sSongTagAwakeningTex,
    sSongTagGoronTex,   sSongTagGoronHalfTex, sSongTagZoraTex,     sSongTagEmptinessTex, sSongTagOrderTex,
};
static const s16 sOotmmSongTagWidths[] = {
    32, 64, 64, 64, 32, 64, 64, 64, 64, 64, 64, 64, 32, 64, 64, 64, 64, 64, 64, 64,
};
static const s16 sOotmmSongTagHeights[] = {
    64, 64, 64, 64, 32, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};
static const Color_RGB8 sOotmmSongTagColors[] = {
    { 255, 255, 0 },   { 255, 136, 0 },  { 61, 168, 29 },   { 163, 163, 0 }, { 238, 34, 34 },
    { 136, 136, 255 }, { 0, 255, 0 },    { 255, 0, 0 },     { 0, 0, 255 },   { 255, 128, 0 },
    { 128, 0, 255 },   { 255, 255, 0 },  { 255, 0, 255 },   { 225, 208, 7 }, { 0, 255, 0 },
    { 156, 132, 41 },  { 156, 132, 41 }, { 132, 198, 198 }, { 255, 128, 0 }, { 255, 255, 255 },
};

static Vtx sOotmmSongTagQuad[] = {
    VTX(-26, 0, -26, 0, 0, 255, 255, 255, 255),
    VTX(-26, 0, 26, 0, 2048, 255, 255, 255, 255),
    VTX(26, 0, 26, 2048, 2048, 255, 255, 255, 255),
    VTX(26, 0, -26, 2048, 0, 255, 255, 255, 255),
};

static void EnOkarinaTag_DrawTexture(PlayState* play, const char* texture, s16 width, s16 height, u8 red, u8 green,
                                     u8 blue) {
    OPEN_DISPS(play->state.gfxCtx);
    gDPLoadTextureBlock(POLY_XLU_DISP++, texture, G_IM_FMT_IA, G_IM_SIZ_8b, width, height, 0,
                        width == 32 ? G_TX_MIRROR : G_TX_NOMIRROR, height == 32 ? G_TX_MIRROR : G_TX_NOMIRROR,
                        width == 32 ? 5 : 6, height == 32 ? 5 : 6, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIDECALA_PRIM, G_CC_PASS2);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_XLU_DECAL2);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_CULL_BACK | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, red, green, blue, 255);
    gSPVertex(POLY_XLU_DISP++, sOotmmSongTagQuad, 4, 0);
    gSP2Triangles(POLY_XLU_DISP++, 0, 2, 1, 0, 0, 3, 2, 0);
    CLOSE_DISPS(play->state.gfxCtx);
}

static Actor* EnOkarinaTag_FindShadowShip(PlayState* play) {
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_BG].head;
    while (actor != NULL && actor->id != ACTOR_BG_HAKA_SHIP) {
        actor = actor->next;
    }
    return actor;
}

static void EnOkarinaTag_DrawOotmm(Actor* thisx, PlayState* play) {
    const s32 song = OotmmSongEvents_Song(thisx, play);
    if (song < 0 || song >= ARRAY_COUNT(sOotmmSongTagTextures)) {
        return;
    }
    Vec3f pos = { thisx->world.pos.x, thisx->world.pos.y + 0.1f, thisx->world.pos.z };
    f32 rot = 0.0f;
    f32 padScale = 0.0f;
    s32 vertical = false;
    switch (play->sceneNum) {
        case SCENE_TEMPLE_OF_TIME:
            pos.z += 15.0f;
            break;
        case SCENE_GRAVEYARD:
            rot = (M_PI / 2.0f) * 3.0f;
            padScale = 2.3f;
            break;
        case SCENE_WINDMILL_AND_DAMPES_GRAVE:
            pos.z += 250.0f;
            break;
        case SCENE_ZORAS_RIVER:
            padScale = 1.2f;
            rot = M_PI / 2.0f;
            break;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS:
        case SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC:
            padScale = 1.2f;
            break;
        case SCENE_BOTTOM_OF_THE_WELL:
            pos.x = 0.0f;
            pos.z = -1480.0f;
            padScale = 3.0f;
            break;
        case SCENE_INSIDE_GANONS_CASTLE:
            rot = M_PI / 2.0f;
            pos.y = -239.9f;
            padScale = 1.2f;
            break;
        case SCENE_WATER_TEMPLE:
            vertical = true;
            padScale = 1.2f;
            if (thisx->room == 0) {
                pos.y = 928.0f;
                pos.z = 350.0f;
            } else if (thisx->room == 1) {
                pos.y = 432.0f;
                pos.z = 0.0f;
            } else if (thisx->room == 17) {
                pos.y = 859.0f;
                pos.z = 340.0f;
            }
            break;
        case SCENE_SHADOW_TEMPLE: {
            Actor* ship = EnOkarinaTag_FindShadowShip(play);
            thisx->flags |= ACTOR_FLAG_DRAW_CULLING_DISABLED;
            if (ship == NULL) {
                return;
            }
            padScale = 4.0f;
            rot = M_PI / 2.0f;
            pos.x = ship->world.pos.x - 85.0f;
            pos.y = ship->world.pos.y + 150.0f;
            pos.z = ship->world.pos.z;
            break;
        }
        case SCENE_SPIRIT_TEMPLE:
            padScale = 1.2f;
            if (thisx->room == 5) {
                rot = M_PI;
                pos.y = 667.0f;
                pos.z = -1250.0f;
            } else if (thisx->room == 24) {
                pos.z = -800.0f;
            }
            break;
    }
    Matrix_Translate(pos.x, pos.y, pos.z, MTXMODE_NEW);
    if (vertical) {
        Matrix_RotateX(M_PI / 2.0f, MTXMODE_APPLY);
    }
    if (rot != 0.0f) {
        Matrix_RotateY(rot, MTXMODE_APPLY);
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    if (padScale != 0.0f) {
        Matrix_Push();
        Matrix_Translate(0.0f, vertical ? 0.0f : -0.05f, vertical ? -0.05f : 0.0f, MTXMODE_APPLY);
        Matrix_Scale(padScale, 1.0f, padScale, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        Matrix_Pop();
        CLOSE_DISPS(play->state.gfxCtx);
        EnOkarinaTag_DrawTexture(play, sSongTagPadTex, 32, 32, 200, 200, 200);
        OPEN_DISPS(play->state.gfxCtx);
    }
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISPS(play->state.gfxCtx);
    EnOkarinaTag_DrawTexture(play, sOotmmSongTagTextures[song], sOotmmSongTagWidths[song], sOotmmSongTagHeights[song],
                             sOotmmSongTagColors[song].r, sOotmmSongTagColors[song].g, sOotmmSongTagColors[song].b);
}

static void EnOkarinaTag_InhibitShuffledSongEffect(PlayState* play) {
    if (OotmmSongEvents_Enabled()) {
        play->msgCtx.ocarinaAction = OCARINA_ACTION_CHECK_NOWARP_DONE;
        play->msgCtx.unk_E40E = 1;
    }
}

void EnOkarinaTag_Destroy(Actor* thisx, PlayState* play) {
}

void EnOkarinaTag_Init(Actor* thisx, PlayState* play) {
    EnOkarinaTag* this = (EnOkarinaTag*)thisx;

    if (OotmmSongEvents_Song(thisx, play) >= 0) {
        thisx->draw = EnOkarinaTag_DrawOotmm;
    }

    osSyncPrintf("\n\n");
    // "Ocarina tag outbreak"
    osSyncPrintf(VT_FGCOL(GREEN) "☆☆☆☆☆ オカリナタグ発生 ☆☆☆☆☆ %x\n" VT_RST, this->actor.params);
    this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    this->type = (this->actor.params >> 0xA) & 0x3F;
    this->ocarinaSong = (this->actor.params >> 6) & 0xF;
    this->switchFlag = this->actor.params & 0x3F;
    if (this->switchFlag == 0x3F) {
        this->switchFlag = -1;
    }
    if (this->ocarinaSong == 0xF) {
        this->ocarinaSong = 0;
        this->unk_158 = 1;
    }
    this->actor.targetMode = 1;
    if (this->actor.world.rot.z > 0) {
        this->interactRange = this->actor.world.rot.z * 40.0f;
    }

    // "Save information"
    osSyncPrintf(VT_FGCOL(GREEN) "☆☆☆☆☆ セーブ情報\t ☆☆☆☆☆ %d\n" VT_RST, this->switchFlag);
    // "Type index"
    osSyncPrintf(VT_FGCOL(YELLOW) "☆☆☆☆☆ 種類インデックス ☆☆☆☆☆ %d\n" VT_RST, this->type);
    // "Correct answer information"
    osSyncPrintf(VT_FGCOL(PURPLE) "☆☆☆☆☆ 正解情報\t ☆☆☆☆☆ %d\n" VT_RST, this->ocarinaSong);
    // "Range information"
    osSyncPrintf(VT_FGCOL(CYAN) "☆☆☆☆☆ 範囲情報\t ☆☆☆☆☆ %d\n" VT_RST, this->actor.world.rot.z);
    // "Processing range information"
    osSyncPrintf(VT_FGCOL(CYAN) "☆☆☆☆☆ 処理範囲情報\t ☆☆☆☆☆ %f\n" VT_RST, this->interactRange);
    // "Hit?"
    osSyncPrintf(VT_FGCOL(GREEN) "☆☆☆☆☆ 当り？\t\t ☆☆☆☆☆ %d\n" VT_RST, this->unk_158);
    osSyncPrintf("\n\n");

    if (GameInteractor_Should(VB_OKARINA_TAG_COMPLETED,
                              (this->switchFlag >= 0) && (Flags_GetSwitch(play, this->switchFlag)), this)) {
        Actor_Kill(&this->actor);
    } else {
        switch (this->type) {
            case 7:
                this->actionFunc = func_80ABEF2C;
                break;
            case 2:
                if (LINK_IS_ADULT) {
                    Actor_Kill(&this->actor);
                    break;
                }
            case 1:
            case 4:
            case 6:
                this->actionFunc = func_80ABF28C;
                break;
            case 5:
                this->actor.textId = 0x5021;
                this->actionFunc = func_80ABF708;
                break;
            default:
                Actor_Kill(&this->actor);
                break;
        }
    }
}

void func_80ABEF2C(EnOkarinaTag* this, PlayState* play) {
    Player* player;
    u16 ocarinaSong;

    player = GET_PLAYER(play);
    this->unk_15A++;
    if (GameInteractor_Should(VB_OKARINA_TAG_COMPLETED,
                              (this->switchFlag >= 0) && (Flags_GetSwitch(play, this->switchFlag)), this)) {
        this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    } else {
        if ((this->ocarinaSong != 6) || (gSaveContext.scarecrowSpawnSongSet)) {
            if (player->stateFlags2 & PLAYER_STATE2_ATTEMPT_PLAY_FOR_ACTOR) {
                // "North! ! ! ! !"
                osSyncPrintf(VT_FGCOL(RED) "☆☆☆☆☆ 北！！！！！ ☆☆☆☆☆ %f\n" VT_RST, this->actor.xzDistToPlayer);
            }
            if ((this->actor.xzDistToPlayer < (90.0f + this->interactRange)) &&
                (fabsf(player->actor.world.pos.y - this->actor.world.pos.y) < 80.0f)) {
                if (player->stateFlags2 & PLAYER_STATE2_PLAY_FOR_ACTOR) {
                    ocarinaSong = this->ocarinaSong;
                    if (ocarinaSong == 6) {
                        ocarinaSong = 0xA;
                    }
                    player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
                    func_8010BD58(play, OotmmSongEvents_Action(&this->actor, play,
                                                               ocarinaSong + OCARINA_ACTION_CHECK_SARIA));
                    this->actionFunc = func_80ABF0CC;
                } else if ((this->actor.xzDistToPlayer < (50.0f + this->interactRange) &&
                            ((fabsf(player->actor.world.pos.y - this->actor.world.pos.y) < 40.0f)))) {
                    this->unk_15A = 0;
                    player->unk_6A8 = &this->actor;
                }
            }
        }
    }
}

void func_80ABF0CC(EnOkarinaTag* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (play->msgCtx.ocarinaMode == OCARINA_MODE_04) {
        this->actionFunc = func_80ABEF2C;
    } else {
        if (play->msgCtx.ocarinaMode == OCARINA_MODE_03) {
            EnOkarinaTag_InhibitShuffledSongEffect(play);
            if (GameInteractor_Should(VB_OKARINA_TAG_COMPLETE, this->switchFlag >= 0, this)) {
                Flags_SetSwitch(play, this->switchFlag);
            }
            if (play->sceneNum == SCENE_WATER_TEMPLE) {
                play->msgCtx.msgMode = MSGMODE_PAUSED;
            }
            if ((play->sceneNum != SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC) &&
                (play->sceneNum != SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS)) {
                play->msgCtx.ocarinaMode = OCARINA_MODE_04;
            }
            Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
            this->actionFunc = func_80ABEF2C;
            return;
        }
        if (this->unk_158 != 0) {
            if ((play->msgCtx.ocarinaMode == OCARINA_MODE_05) || (play->msgCtx.ocarinaMode == OCARINA_MODE_06) ||
                (play->msgCtx.ocarinaMode == OCARINA_MODE_07) || (play->msgCtx.ocarinaMode == OCARINA_MODE_08) ||
                (play->msgCtx.ocarinaMode == OCARINA_MODE_09) || (play->msgCtx.ocarinaMode == OCARINA_MODE_0A) ||
                (play->msgCtx.ocarinaMode == OCARINA_MODE_0D)) {
                if (GameInteractor_Should(VB_OKARINA_TAG_COMPLETE, this->switchFlag >= 0, this)) {
                    Flags_SetSwitch(play, this->switchFlag);
                }
                play->msgCtx.ocarinaMode = OCARINA_MODE_04;
                Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
                this->actionFunc = func_80ABEF2C;
                return;
            }
        }
        if ((play->msgCtx.ocarinaMode >= OCARINA_MODE_05) && (play->msgCtx.ocarinaMode < OCARINA_MODE_0E)) {
            play->msgCtx.ocarinaMode = OCARINA_MODE_04;
            this->actionFunc = func_80ABEF2C;
        } else if (play->msgCtx.ocarinaMode == OCARINA_MODE_01) {
            player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
        }
    }
}

void func_80ABF28C(EnOkarinaTag* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    this->unk_15A++;
    if ((this->ocarinaSong != 6) || (gSaveContext.scarecrowSpawnSongSet)) {
        if (GameInteractor_Should(VB_OKARINA_TAG_COMPLETED,
                                  (this->switchFlag >= 0) && (Flags_GetSwitch(play, this->switchFlag)), this)) {
            this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        } else if (((this->type != 4) ||
                    GameInteractor_Should(VB_BE_ELIGIBLE_TO_OPEN_DOT,
                                          !Flags_GetEventChkInf(EVENTCHKINF_OPENED_THE_DOOR_OF_TIME), this)) &&
                   ((this->type != 6) || !Flags_GetEventChkInf(EVENTCHKINF_DESTROYED_ROYAL_FAMILY_TOMB)) &&
                   (this->actor.xzDistToPlayer < (90.0f + this->interactRange)) &&
                   (fabsf(player->actor.world.pos.y - this->actor.world.pos.y) < 80.0f)) {
            if (player->stateFlags2 & PLAYER_STATE2_ATTEMPT_PLAY_FOR_ACTOR) {
                switch (this->type) {
                    case 1:
                        func_8010BD58(play, OotmmSongEvents_Action(&this->actor, play,
                                                                   OCARINA_ACTION_CHECK_LULLABY));
                        break;
                    case 2:
                        func_8010BD58(play, OotmmSongEvents_Action(&this->actor, play,
                                                                   OCARINA_ACTION_CHECK_STORMS));
                        break;
                    case 4:
                        func_8010BD58(play, OotmmSongEvents_Action(&this->actor, play,
                                                                   OCARINA_ACTION_CHECK_TIME));
                        break;
                    case 6:
                        func_8010BD58(play, OotmmSongEvents_Action(&this->actor, play,
                                                                   OCARINA_ACTION_CHECK_LULLABY));
                        break;
                    default:
                        // "Ocarina Invisible-kun demo start check error source"
                        osSyncPrintf(VT_FGCOL(GREEN) "☆☆☆☆☆ オカリナ透明君デモ開始チェックエラー原 ☆☆☆☆☆ %d\n" VT_RST,
                                     this->type);
                        Actor_Kill(&this->actor);
                        break;
                }
                player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
                this->actionFunc = func_80ABF4C8;
            } else if ((this->actor.xzDistToPlayer < (50.0f + this->interactRange)) &&
                       (fabsf(player->actor.world.pos.y - this->actor.world.pos.y) < 40.0f)) {
                this->unk_15A = 0;
                player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
            }
        }
    }
}

void func_80ABF4C8(EnOkarinaTag* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (play->msgCtx.ocarinaMode == OCARINA_MODE_04) {
        this->actionFunc = func_80ABF28C;
    } else if (play->msgCtx.ocarinaMode == OCARINA_MODE_03) {
        EnOkarinaTag_InhibitShuffledSongEffect(play);
        Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
        if (GameInteractor_Should(VB_OKARINA_TAG_COMPLETE, this->switchFlag >= 0, this)) {
            Flags_SetSwitch(play, this->switchFlag);
        }
        switch (this->type) {
            case 1: // Zora's River Waterfall
                Flags_SetSwitch(play, this->switchFlag);
                Flags_SetEventChkInf(EVENTCHKINF_OPENED_ZORAS_DOMAIN);
                break;
            case 2: // Kakariko Windmill
                if (GameInteractor_Should(VB_PLAY_DRAIN_WELL_CS, true, this)) {
                    play->csCtx.segment = D_80ABF9D0;
                    gSaveContext.cutsceneTrigger = 1;
                }
                func_800F574C(1.18921f, 0x5A);
                break;
            case 4: // Door of Time
                if (GameInteractor_Should(VB_PLAY_DOOR_OF_TIME_CS, true, this)) {
                    play->csCtx.segment = D_80ABFB40;
                    gSaveContext.cutsceneTrigger = 1;
                }
                break;
            case 6: // Royal Family Tomb
                if (GameInteractor_Should(VB_PLAY_ROYAL_FAMILY_TOMB_CS, true, this)) {
                    play->csCtx.segment = LINK_IS_ADULT ? SEGMENTED_TO_VIRTUAL(&spot02_scene_Cs_003C80)
                                                        : SEGMENTED_TO_VIRTUAL(&spot02_scene_Cs_005020);
                    gSaveContext.cutsceneTrigger = 1;
                }
                Flags_SetEventChkInf(EVENTCHKINF_DESTROYED_ROYAL_FAMILY_TOMB);
                Sfx_PlaySfxCentered(NA_SE_SY_CORRECT_CHIME);
                break;
            default:
                break;
        }
        play->msgCtx.ocarinaMode = OCARINA_MODE_04;
        this->actionFunc = func_80ABF28C;
    } else {
        if (play->msgCtx.ocarinaMode >= OCARINA_MODE_05) {
            if (play->msgCtx.ocarinaMode < OCARINA_MODE_0E) {
                play->msgCtx.ocarinaMode = OCARINA_MODE_04;
                this->actionFunc = func_80ABF28C;
                return;
            }
        }
        if (play->msgCtx.ocarinaMode == OCARINA_MODE_01) {
            player->stateFlags2 |= PLAYER_STATE2_NEAR_OCARINA_ACTOR;
        }
    }
}

void func_80ABF708(EnOkarinaTag* this, PlayState* play) {
    s16 yawDiff;
    s16 yawDiffNew;

    if (Actor_ProcessTalkRequest(&this->actor, play)) {
        this->actionFunc = func_80ABF7CC;
    } else {
        yawDiff = this->actor.yawTowardsPlayer - this->actor.world.rot.y;
        this->unk_15A++;
        if (!(this->actor.xzDistToPlayer > 120.0f)) {
            if (CHECK_QUEST_ITEM(QUEST_SONG_SUN)) {
                this->actor.textId = 0x5021;
            }
            yawDiffNew = ABS(yawDiff);
            if (yawDiffNew < 0x4300) {
                this->unk_15A = 0;
                func_8002F2CC(&this->actor, play, 70.0f);
            }
        }
    }
}

void func_80ABF7CC(EnOkarinaTag* this, PlayState* play) {
    // "Open sesame sesame!"
    osSyncPrintf(VT_FGCOL(PURPLE) "☆☆☆☆☆ 開けゴマゴマゴマ！ ☆☆☆☆☆ %d\n" VT_RST, Message_GetState(&play->msgCtx));

    if ((Message_GetState(&play->msgCtx) == TEXT_STATE_EVENT) && Message_ShouldAdvance(play)) {
        Message_CloseTextbox(play);
        if (GameInteractor_Should(VB_PLAY_SUNS_SONG_CS, !CHECK_QUEST_ITEM(QUEST_SONG_SUN), this)) {
            play->csCtx.segment = SEGMENTED_TO_VIRTUAL(&gSunSongGraveSunSongTeachCs);
            gSaveContext.cutsceneTrigger = 1;
        }
        this->actionFunc = func_80ABF708;
    }
}

void EnOkarinaTag_Update(Actor* thisx, PlayState* play) {
    EnOkarinaTag* this = (EnOkarinaTag*)thisx;

    this->actionFunc(this, play);
    if (BREG(0) != 0) {
        if (this->unk_15A != 0) {
            if (!(this->unk_15A & 1)) {
                DebugDisplay_AddObject(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z,
                                       this->actor.world.rot.x, this->actor.world.rot.y, this->actor.world.rot.z, 1.0f,
                                       1.0f, 1.0f, 120, 120, 120, 255, 4, play->state.gfxCtx);
            }
        } else {
            DebugDisplay_AddObject(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z,
                                   this->actor.world.rot.x, this->actor.world.rot.y, this->actor.world.rot.z, 1.0f,
                                   1.0f, 1.0f, 255, 0, 0, 255, 4, play->state.gfxCtx);
        }
    }
}
