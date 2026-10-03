#include "OotmmActorSpawn.h"

#include "OotmmSession.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
}

namespace {

constexpr int16_t kEntranceChildArchery = 0x16D;
constexpr int16_t kEntranceAdultArchery = 0x3B;

bool HasSpiritualStones() {
    return CHECK_QUEST_ITEM(QUEST_KOKIRI_EMERALD) && CHECK_QUEST_ITEM(QUEST_GORON_RUBY) &&
           CHECK_QUEST_ITEM(QUEST_ZORA_SAPPHIRE);
}

} // namespace

extern "C" int32_t OotmmActorSpawn_Rewrite(PlayState* play, int16_t* actorId, int16_t* params) {
    if (play == nullptr || !OotmmSession_IsActive()) {
        return 1;
    }
    const uint16_t variable = static_cast<uint16_t>(*params);
    switch (*actorId) {
        case ACTOR_EN_SYATEKI_MAN:
            return (gSaveContext.entranceIndex == kEntranceChildArchery && LINK_IS_CHILD) ||
                   (gSaveContext.entranceIndex == kEntranceAdultArchery && LINK_IS_ADULT);
        case ACTOR_BG_JYA_BLOCK:
            return !OotmmSession_BoolSetting("agelessStrength");
        case ACTOR_ITEM_OCARINA:
            return HasSpiritualStones();
        case ACTOR_EN_MM:
            return variable != 0x01 || HasSpiritualStones();
        case ACTOR_OBJ_HANA:
            if ((variable & 3) == 1) {
                *actorId = ACTOR_EN_ISHI;
                *params = 0x200;
            } else if ((variable & 3) == 2) {
                *actorId = ACTOR_EN_KUSA;
                *params = static_cast<int16_t>(0xFF00);
            }
            return 1;
        case ACTOR_EN_DOOR:
            if (play->sceneNum == SCENE_MARKET_NIGHT && OotmmSession_BoolSetting("openMaskShop") &&
                ((variable >> 7) & 7) == 5 && (variable & 0x3F) == 0x10) {
                *params = static_cast<int16_t>((variable & 0xFC00) | 0x1BF);
            }
            return 1;
        case ACTOR_BG_SPOT06_OBJECTS:
            return play->sceneNum != SCENE_LAKE_HYLIA || !OotmmSession_BoolSetting("openZdShortcut") ||
                   ((variable >> 8) & 0xFF) != 3;
        default:
            return 1;
    }
}
