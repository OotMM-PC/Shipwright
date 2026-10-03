#include "OotmmBrokenActors.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

extern "C" {
#include "z64.h"
#include "functions.h"
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
    REGISTER_VB_SHOULD(VB_SPAWN_ACTOR_ENTRY, {
        ActorContext* actorCtx = va_arg(args, ActorContext*);
        ActorEntry* entry = va_arg(args, ActorEntry*);
        PlayState* play = va_arg(args, PlayState*);
        Actor** spawned = va_arg(args, Actor**);
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
