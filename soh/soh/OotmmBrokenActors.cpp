#include "OotmmBrokenActors.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

extern "C" {
#include "z64.h"
#include "functions.h"
extern PlayState* gPlayState;
}

namespace {

enum class Layout { Vanilla, MasterQuest };

constexpr s32 kRemove = -1;

struct ActorFix {
    s16 Scene;
    Layout Dungeon;
    s16 Id;
    s16 X;
    s16 Y;
    s16 Z;
    s32 Params;
    bool Always;
};

constexpr ActorFix kFixes[] = {
    { SCENE_DODONGOS_CAVERN, Layout::Vanilla, ACTOR_OBJ_HSBLOCK, -2083, 444, -1858, kRemove, false },
    { SCENE_DODONGOS_CAVERN, Layout::Vanilla, ACTOR_OBJ_HSBLOCK, -1835, 443, -1858, kRemove, false },
    { SCENE_DODONGOS_CAVERN, Layout::Vanilla, ACTOR_EN_KUSA, 2370, 0, 216, kRemove, false },
    { SCENE_WATER_TEMPLE, Layout::Vanilla, ACTOR_EN_FIREFLY, -1645, 249, -74, kRemove, false },
    { SCENE_WATER_TEMPLE, Layout::Vanilla, ACTOR_EN_FIREFLY, -1594, 312, -298, kRemove, false },
    { SCENE_SPIRIT_TEMPLE, Layout::Vanilla, ACTOR_OBJ_HSBLOCK, 680, 1843, 246, kRemove, false },
    { SCENE_SPIRIT_TEMPLE, Layout::Vanilla, ACTOR_OBJ_TSUBO, -1055, 30, -1680, 0x603F, false },
    { SCENE_INSIDE_GANONS_CASTLE, Layout::Vanilla, ACTOR_MIR_RAY, -1175, 150, 1196, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_FIREFLY, -563, 75, 351, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_FIREFLY, -563, 75, 709, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_FIREFLY, -276, 131, -148, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_FIREFLY, -39, 133, 150, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_FIREFLY, -25, 133, -81, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_SW, -686, 137, -158, kRemove, false },
    { SCENE_GRAVEYARD, Layout::Vanilla, ACTOR_EN_SW, -38, 135, 405, kRemove, false },
    { SCENE_LAKE_HYLIA, Layout::Vanilla, ACTOR_EN_KUSA, -75, -1253, 2664, kRemove, false },
    { SCENE_LAKE_HYLIA, Layout::Vanilla, ACTOR_OBJ_TSUBO, -2378, -1033, 3928, kRemove, false },
    { SCENE_LAKE_HYLIA, Layout::Vanilla, ACTOR_OBJ_TSUBO, -2378, -1033, 3967, kRemove, false },
    { SCENE_LOST_WOODS, Layout::Vanilla, ACTOR_EN_OKUTA, 2114, -20, -847, kRemove, false },
    { SCENE_GERUDOS_FORTRESS, Layout::Vanilla, ACTOR_EN_KANBAN, 980, 654, -480, kRemove, false },
    { SCENE_GERUDOS_FORTRESS, Layout::Vanilla, ACTOR_EN_KANBAN, 14, 333, -1159, kRemove, false },
    { SCENE_GERUDOS_FORTRESS, Layout::Vanilla, ACTOR_EN_KANBAN, -1850, 9, -3250, kRemove, false },
    { SCENE_HYRULE_CASTLE, Layout::Vanilla, ACTOR_OBJ_TSUBO, 1140, 1128, 3104, kRemove, false },
    { SCENE_HYRULE_CASTLE, Layout::Vanilla, ACTOR_OBJ_TSUBO, 1140, 1128, 3141, kRemove, false },
    { SCENE_DODONGOS_CAVERN, Layout::MasterQuest, ACTOR_EN_KUSA, 1713, 411, -1246, kRemove, false },
    { SCENE_DODONGOS_CAVERN, Layout::MasterQuest, ACTOR_EN_KUSA, 3189, 411, -485, kRemove, false },
    { SCENE_JABU_JABU, Layout::MasterQuest, ACTOR_EN_RU1, 6, -340, -4354, kRemove, false },
    { SCENE_SPIRIT_TEMPLE, Layout::MasterQuest, ACTOR_EN_FIREFLY, -1156, 72, -501, kRemove, false },
    { SCENE_SPIRIT_TEMPLE, Layout::MasterQuest, ACTOR_EN_FIREFLY, -965, 72, -501, kRemove, false },
    { SCENE_SPIRIT_TEMPLE, Layout::MasterQuest, ACTOR_EN_SW, 681, 1200, -87, kRemove, false },
    { SCENE_SHADOW_TEMPLE, Layout::MasterQuest, ACTOR_OBJ_ROOMTIMER, 3358, -543, 608, 0xFFFF, true },
    { SCENE_SHADOW_TEMPLE, Layout::MasterQuest, ACTOR_OBJ_ROOMTIMER, 3358, -543, -466, 0xFFFF, true },
};

struct ActorMove {
    s16 Scene;
    s16 Id;
    s16 X;
    s16 Y;
    s16 Z;
    s16 NewX;
    s16 NewY;
    s16 NewZ;
    s16 NewRotY;
    s32 NewParams;
};

constexpr s32 kKeepParams = -1;
constexpr s16 kKeepRot = 0x7FFF;

constexpr ActorMove kMoves[] = {
    { SCENE_FOREST_TEMPLE, ACTOR_OBJ_SWITCH, 797, -779, -1566, 797, -780, -1566, kKeepRot, kKeepParams },
    { SCENE_FOREST_TEMPLE, ACTOR_OBJ_SWITCH, 119, -779, -889, 119, -780, -889, kKeepRot, kKeepParams },
    { SCENE_FOREST_TEMPLE, ACTOR_OBJ_SWITCH, -346, -779, -2030, -346, -780, -2030, kKeepRot, kKeepParams },
    { SCENE_FIRE_TEMPLE, ACTOR_OBJ_SWITCH, -2005, 4500, -331, -2005, 4499, -331, kKeepRot, kKeepParams },
    { SCENE_SHADOW_TEMPLE, ACTOR_EN_RD, 2839, -1343, 1217, 2839, -1343, 1217, kKeepRot, 0x0002 },
    { SCENE_SHADOW_TEMPLE, ACTOR_EN_RD, 2121, -1343, 1183, 2121, -1343, 1183, kKeepRot, 0x0002 },
    { SCENE_KOKIRI_FOREST, ACTOR_EN_WONDER_ITEM, 364, 0, 28, 370, 5, -180, kKeepRot, kKeepParams },
    { SCENE_GERUDOS_FORTRESS, ACTOR_EN_GE1, -857, 112, -3123, -1336, 93, -3000, static_cast<s16>(0x95B0), 0x0301 },
};

struct ActorSkip {
    s16 Scene;
    s16 Id;
    s16 X;
    s16 Y;
    s16 Z;
};

constexpr ActorSkip kFireChildRocks[] = {
    { SCENE_DEATH_MOUNTAIN_CRATER, ACTOR_EN_ISHI, -20, 472, -659 },
    { SCENE_DEATH_MOUNTAIN_CRATER, ACTOR_EN_ISHI, 38, 471, -627 },
    { SCENE_DEATH_MOUNTAIN_CRATER, ACTOR_EN_ISHI, 92, 469, -610 },
};

const ActorFix* Find(const PlayState* play, const ActorEntry* entry) {
    const Layout layout =
        OotmmSession_IsSceneMasterQuest(play->sceneNum) ? Layout::MasterQuest : Layout::Vanilla;
    for (const auto& fix : kFixes) {
        if (fix.Scene == play->sceneNum && fix.Dungeon == layout && fix.Id == entry->id && fix.X == entry->pos.x &&
            fix.Y == entry->pos.y && fix.Z == entry->pos.z) {
            return &fix;
        }
    }
    return nullptr;
}

} // namespace

void OotmmBrokenActors_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        if (!OotmmSession_IsActive() || gPlayState == nullptr || gPlayState->colCtx.colHeader == nullptr) {
            return;
        }
        CollisionHeader* header = gPlayState->colCtx.colHeader;
        if (sceneNum == SCENE_ICE_CAVERN && header->cameraDataList != nullptr && header->cameraDataListLen >= 2) {
            header->cameraDataList[0].cameraSType = 0x0001;
            header->cameraDataList[1].cameraSType = 0x0001;
        } else if (sceneNum == SCENE_GRAVEYARD && header->surfaceTypeList != nullptr) {
            header->surfaceTypeList[0].data[0] |= 0x00200000;
            header->surfaceTypeList[20].data[0] |= 0x24000000;
        }
    });
    REGISTER_VB_SHOULD(VB_SPAWN_ACTOR_ENTRY, {
        ActorContext* actorCtx = va_arg(args, ActorContext*);
        ActorEntry* entry = va_arg(args, ActorEntry*);
        PlayState* play = va_arg(args, PlayState*);
        Actor** spawned = va_arg(args, Actor**);
        if (OotmmSession_IsActive() && !OotmmSession_IsSceneMasterQuest(play->sceneNum)) {
            for (const auto& move : kMoves) {
                if (move.Scene == play->sceneNum && move.Id == entry->id && move.X == entry->pos.x &&
                    move.Y == entry->pos.y && move.Z == entry->pos.z) {
                    *should = false;
                    *spawned = Actor_Spawn(actorCtx, play, entry->id, move.NewX, move.NewY, move.NewZ, entry->rot.x,
                                           move.NewRotY == kKeepRot ? entry->rot.y : move.NewRotY, entry->rot.z,
                                           move.NewParams == kKeepParams ? entry->params
                                                                         : static_cast<s16>(move.NewParams));
                    return;
                }
            }
            if (OotmmSession_GetState().WorldFlagContains("openDungeonsOot", "fireChild")) {
                for (const auto& rock : kFireChildRocks) {
                    if (rock.Scene == play->sceneNum && rock.Id == entry->id && rock.X == entry->pos.x &&
                        rock.Y == entry->pos.y && rock.Z == entry->pos.z) {
                        *should = false;
                        *spawned = nullptr;
                        return;
                    }
                }
            }
        }
        const ActorFix* fix = Find(play, entry);
        if (fix == nullptr || (!fix->Always && OotmmSession_BoolSetting("restoreBrokenActors"))) {
            return;
        }
        *should = false;
        *spawned = fix->Params == kRemove
                       ? nullptr
                       : Actor_Spawn(actorCtx, play, entry->id, entry->pos.x, entry->pos.y, entry->pos.z,
                                     entry->rot.x, entry->rot.y, entry->rot.z, static_cast<s16>(fix->Params));
    });
}
