#include "OotmmSouls.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

#include <array>
#include <string>
#include <string_view>

extern "C" {
#include "functions.h"
#include "variables.h"
#include "z64.h"
}

namespace {

struct SoulCategory {
    std::string_view Prefix;
    const char* EnableSetting;
    const char* SharedSetting;
};

constexpr std::array<SoulCategory, 5> kCategories = { {
    { "ENEMY_", "soulsEnemyOot", "sharedSoulsEnemy" },
    { "BOSS_", "soulsBossOot", nullptr },
    { "NPC_", "soulsNpcOot", "sharedSoulsNpc" },
    { "ANIMAL_", "soulsAnimalOot", "sharedSoulsAnimal" },
    { "MISC_", "soulsMiscOot", "sharedSoulsMisc" },
} };

bool sRoomHidesEnemy = false;

const SoulCategory* CategoryOf(std::string_view soul) {
    for (const SoulCategory& category : kCategories) {
        if (soul.starts_with(category.Prefix)) {
            return &category;
        }
    }
    return nullptr;
}

bool OwnsSoul(std::string_view soul) {
    const SoulCategory* category = CategoryOf(soul);
    const auto& state = OotmmSession_GetState();
    if (category == nullptr || !state.GetBoolSetting(category->EnableSetting, false)) {
        return true;
    }

    const auto& inventory = OotmmIpc_GetInventory();
    if (category->SharedSetting != nullptr && state.GetBoolSetting(category->SharedSetting, false) &&
        inventory.Has("SHARED_SOUL_" + std::string(soul))) {
        return true;
    }
    return inventory.Has("OOT_SOUL_" + std::string(soul));
}

enum class SpawnVerdict {
    Allow,
    Block,
    BlockAndHoldRoom,
};

SpawnVerdict Gate(std::string_view soul) {
    return OwnsSoul(soul) ? SpawnVerdict::Allow : SpawnVerdict::Block;
}

SpawnVerdict GateEnemy(std::string_view soul) {
    return OwnsSoul(soul) ? SpawnVerdict::Allow : SpawnVerdict::BlockAndHoldRoom;
}

SpawnVerdict ResolveShopkeeper(uint16_t params) {
    switch (params) {
        case 0:
            return Gate("NPC_KOKIRI_SHOPKEEPER");
        case 1:
        case 3:
            return Gate("NPC_POTION_SHOPKEEPER");
        case 2:
            return Gate("NPC_BOMBCHU_SHOPKEEPER");
        case 4:
            return Gate("NPC_BAZAAR_SHOPKEEPER");
        case 7:
            return Gate("NPC_ZORA_SHOPKEEPER");
        case 8:
            return Gate("NPC_GORON_SHOPKEEPER");
        default:
            return SpawnVerdict::Allow;
    }
}

SpawnVerdict ResolveTownfolk(uint16_t params) {
    switch (params & 0x3F) {
        case 0x00:
            return Gate("NPC_DOG_LADY");
        case 0x05:
            return Gate("NPC_BANKER");
        case 0x07:
            return Gate("NPC_ASTRONOMER");
        default:
            return Gate("NPC_CITIZEN");
    }
}

SpawnVerdict ResolveGoron(uint16_t params) {
    switch (params & 0x1F) {
        case 1:
            return Gate("NPC_GORON_CHILD");
        case 2:
            return Gate("NPC_BIGGORON");
        default:
            return Gate("NPC_GORON");
    }
}

SpawnVerdict ResolveSpawn(PlayState* play, int16_t actorId, uint16_t params) {
    switch (actorId) {
        case ACTOR_EN_DS:
            return Gate("NPC_OLD_HAG");
        case ACTOR_EN_JS:
            return Gate("NPC_CARPET_MAN");
        case ACTOR_EN_HS:
        case ACTOR_EN_HS2:
            return Gate("NPC_GROG");
        case ACTOR_EN_IN:
            return Gate("NPC_GORMAN");
        case ACTOR_EN_MK:
            return Gate("NPC_SCIENTIST");
        case ACTOR_EN_MS:
            return Gate("NPC_BEAN_SALESMAN");
        case ACTOR_EN_TG:
            return Gate("NPC_HONEY_DARLING");
        case ACTOR_EN_TAKARA_MAN:
            return Gate("NPC_CHEST_GAME_OWNER");
        case ACTOR_EN_POH:
            if (params == 2 || params == 3) {
                return Gate("NPC_COMPOSER_BROS");
            }
            return GateEnemy("ENEMY_POE");
        case ACTOR_EN_PO_SISTERS:
        case ACTOR_EN_PO_FIELD:
            return GateEnemy("ENEMY_POE");
        case ACTOR_EN_DNS:
        case ACTOR_EN_SHOPNUTS:
            return Gate("MISC_BUSINESS_SCRUB");
        case ACTOR_EN_TEST:
            return GateEnemy("ENEMY_STALFOS");
        case ACTOR_BG_BDAN_OBJECTS:
            if (params & 0xFF) {
                return SpawnVerdict::Allow;
            }
            [[fallthrough]];
        case ACTOR_EN_OKUTA:
        case ACTOR_EN_BIGOKUTA:
            return GateEnemy("ENEMY_OCTOROK");
        case ACTOR_EN_WALLMAS:
            return GateEnemy("ENEMY_WALLMASTER");
        case ACTOR_EN_DODONGO:
            return GateEnemy("ENEMY_DODONGO");
        case ACTOR_EN_FIREFLY:
            return GateEnemy("ENEMY_KEESE");
        case ACTOR_EN_TITE:
            return GateEnemy("ENEMY_TEKTITE");
        case ACTOR_EN_PEEHAT:
            return GateEnemy("ENEMY_PEAHAT");
        case ACTOR_EN_ZF:
            return GateEnemy("ENEMY_LIZALFOS_DINOLFOS");
        case ACTOR_EN_GOMA:
            if (play->sceneNum == SCENE_DEKU_TREE_BOSS) {
                return SpawnVerdict::Allow;
            }
            return GateEnemy("ENEMY_GOHMA_LARVA");
        case ACTOR_EN_BUBBLE:
            return GateEnemy("ENEMY_SHABOM");
        case ACTOR_EN_DODOJR:
            return GateEnemy("ENEMY_BABY_DODONGO");
        case ACTOR_EN_TORCH2:
            return GateEnemy("ENEMY_DARK_LINK");
        case ACTOR_EN_BILI:
        case ACTOR_EN_VALI:
            return GateEnemy("ENEMY_BIRI_BARI");
        case ACTOR_EN_TP:
            return GateEnemy("ENEMY_TAILPASARN");
        case ACTOR_EN_ST:
            return GateEnemy("ENEMY_SKULLTULA");
        case ACTOR_EN_BW:
            return GateEnemy("ENEMY_TORCH_SLUG");
        case ACTOR_EN_MB:
            return GateEnemy("ENEMY_MOBLIN");
        case ACTOR_EN_AM:
            if (params == 0) {
                return SpawnVerdict::Allow;
            }
            return GateEnemy("ENEMY_ARMOS");
        case ACTOR_EN_DEKUBABA:
        case ACTOR_EN_KAREBABA:
            return GateEnemy("ENEMY_DEKU_BABA");
        case ACTOR_EN_DEKUNUTS:
        case ACTOR_EN_HINTNUTS:
            return GateEnemy("ENEMY_DEKU_SCRUB");
        case ACTOR_EN_BB:
            return GateEnemy("ENEMY_BUBBLE");
        case ACTOR_EN_VM:
            return GateEnemy("ENEMY_BEAMOS");
        case ACTOR_EN_FLOORMAS:
            return GateEnemy("ENEMY_FLOORMASTER");
        case ACTOR_EN_RD:
            return GateEnemy("ENEMY_REDEAD_GIBDO");
        case ACTOR_EN_SW:
            if (params & 0xE000) {
                return Gate("MISC_GS");
            }
            return GateEnemy("ENEMY_SKULLWALLTULA");
        case ACTOR_EN_FD:
            return GateEnemy("ENEMY_FLARE_DANCER");
        case ACTOR_EN_DH:
        case ACTOR_EN_DHA:
            return GateEnemy("ENEMY_DEAD_HAND");
        case ACTOR_EN_SB:
            return GateEnemy("ENEMY_SHELL_BLADE");
        case ACTOR_EN_RR:
            return GateEnemy("ENEMY_LIKE_LIKE");
        case ACTOR_EN_NY:
            return GateEnemy("ENEMY_SPIKE");
        case ACTOR_EN_ANUBICE_TAG:
        case ACTOR_EN_ANUBICE:
            return GateEnemy("ENEMY_ANUBIS");
        case ACTOR_EN_IK:
            return GateEnemy("ENEMY_IRON_KNUCKLE");
        case ACTOR_EN_SKJ:
            if (gSaveContext.linkAge == LINK_AGE_CHILD) {
                return SpawnVerdict::Allow;
            }
            return GateEnemy("ENEMY_SKULL_KID");
        case ACTOR_EN_TUBO_TRAP:
            return Gate("ENEMY_FLYING_POT");
        case ACTOR_EN_FZ:
            return GateEnemy("ENEMY_FREEZARD");
        case ACTOR_EN_WEIYER:
        case ACTOR_EN_EIYER:
            return GateEnemy("ENEMY_STINGER");
        case ACTOR_EN_WF:
            return GateEnemy("ENEMY_WOLFOS");
        case ACTOR_EN_CROW:
            return GateEnemy("ENEMY_GUAY");
        case ACTOR_BOSS_GOMA:
            return GateEnemy("BOSS_QUEEN_GOHMA");
        case ACTOR_BOSS_DODONGO:
            return GateEnemy("BOSS_KING_DODONGO");
        case ACTOR_BOSS_VA:
            return GateEnemy("BOSS_BARINADE");
        case ACTOR_BOSS_GANONDROF:
            return GateEnemy("BOSS_PHANTOM_GANON");
        case ACTOR_BOSS_FD:
            return GateEnemy("BOSS_VOLVAGIA");
        case ACTOR_BOSS_MO:
            return GateEnemy("BOSS_MORPHA");
        case ACTOR_BOSS_SST:
            return GateEnemy("BOSS_BONGO_BONGO");
        case ACTOR_BOSS_TW:
            return GateEnemy("BOSS_TWINROVA");
        case ACTOR_EN_BA:
            return GateEnemy("ENEMY_PARASITE");
        case ACTOR_EN_REEBA:
            return GateEnemy("ENEMY_LEEVER");
        case ACTOR_EN_SKB:
            return GateEnemy("ENEMY_STALCHILD");
        case ACTOR_EN_SA:
            return Gate("NPC_SARIA");
        case ACTOR_EN_DU:
            return Gate("NPC_DARUNIA");
        case ACTOR_EN_RU1:
            return Gate("NPC_RUTO");
        case ACTOR_EN_KZ:
            return Gate("NPC_KING_ZORA");
        case ACTOR_EN_NIW_LADY:
            return Gate("NPC_ANJU");
        case ACTOR_EN_TORYO:
        case ACTOR_EN_DAIKU:
        case ACTOR_EN_DAIKU_KAKARIKO:
            return Gate("NPC_CARPENTERS");
        case ACTOR_EN_FU:
            return Gate("NPC_GURU_GURU");
        case ACTOR_EN_MD:
            return Gate("NPC_MIDO");
        case ACTOR_EN_KO:
            return Gate("NPC_KOKIRI");
        case ACTOR_EN_OSSAN:
            return ResolveShopkeeper(params);
        case ACTOR_EN_HEISHI2:
        case ACTOR_EN_HEISHI3:
        case ACTOR_EN_HEISHI4:
            return Gate("NPC_HYLIAN_GUARD");
        case ACTOR_EN_ANI:
            return Gate("NPC_ROOFTOP_MAN");
        case ACTOR_EN_MU:
        case ACTOR_EN_NIW_GIRL:
        case ACTOR_EN_MM:
        case ACTOR_EN_MM2:
            return Gate("NPC_CITIZEN");
        case ACTOR_EN_HY:
            return ResolveTownfolk(params);
        case ACTOR_EN_MA1:
        case ACTOR_EN_MA2:
        case ACTOR_EN_MA3:
            return Gate("NPC_MALON");
        case ACTOR_BG_SPOT15_RRBOX:
            if (play->sceneNum != SCENE_HYRULE_CASTLE) {
                return SpawnVerdict::Allow;
            }
            [[fallthrough]];
        case ACTOR_EN_TA:
            return Gate("NPC_TALON");
        case ACTOR_FISHING:
            return Gate("NPC_FISHING_POND_OWNER");
        case ACTOR_EN_GO:
            return Gate("NPC_GORON");
        case ACTOR_EN_GO2:
            return ResolveGoron(params);
        case ACTOR_EN_GM:
            return Gate("NPC_MEDIGORON");
        case ACTOR_EN_ZO:
        case ACTOR_EN_DIVING_GAME:
            return Gate("NPC_ZORA");
        case ACTOR_EN_BOM_BOWL_MAN:
        case ACTOR_BG_BOWL_WALL:
            return Gate("NPC_BOMBCHU_BOWLING_LADY");
        case ACTOR_EN_SYATEKI_MAN:
            return Gate("NPC_SHOOTING_GALLERY_OWNER");
        case ACTOR_EN_TK:
        case ACTOR_EN_PO_RELAY:
            return Gate("NPC_DAMPE");
        case ACTOR_EN_CS:
            return Gate("NPC_BOMBERS");
        case ACTOR_EN_GB:
            return Gate("NPC_POE_COLLECTOR");
        case ACTOR_EN_XC:
            return Gate("NPC_SHEIK");
        case ACTOR_EN_ZL1:
        case ACTOR_EN_ZL3:
        case ACTOR_EN_ZL4:
            return Gate("NPC_ZELDA");
        case ACTOR_EN_GE1:
        case ACTOR_EN_GE2:
            return Gate("NPC_THIEVES");
        case ACTOR_EN_GELDB:
            return GateEnemy("ENEMY_THIEVES");
        case ACTOR_EN_NIW:
        case ACTOR_EN_NWC:
            return Gate("ANIMAL_CUCCO");
        case ACTOR_EN_COW:
            return Gate("ANIMAL_COW");
        case ACTOR_EN_DOG:
            return Gate("ANIMAL_DOG");
        case ACTOR_EN_BUTTE:
            return Gate("ANIMAL_BUTTERFLY");
        case ACTOR_ITEM_OCARINA:
            return Gate("NPC_ZELDA");
        case ACTOR_EN_RIVER_SOUND:
            if (play->sceneNum == SCENE_MARKET_DAY && (params & 0xFF) == 0x0A) {
                return Gate("NPC_CITIZEN");
            }
            return SpawnVerdict::Allow;
        default:
            return SpawnVerdict::Allow;
    }
}

void ReplaceMissingActor(PlayState* play, int16_t actorId) {
    switch (actorId) {
        case ACTOR_BOSS_MO:
            // unk_74[0] is the water opacity the Water Temple boss draw config reads.
            play->roomCtx.unk_74[0] = 0xFF;
            if (play->colCtx.colHeader != nullptr && play->colCtx.colHeader->numWaterBoxes > 0) {
                play->colCtx.colHeader->waterBoxes[0].ySurface = -500;
            }
            break;
        case ACTOR_BOSS_SST:
            Actor_Spawn(&play->actorCtx, play, ACTOR_BG_SST_FLOOR, -50.0f, 0.0f, 0.0f, 0, 0, 0, 0);
            break;
        default:
            break;
    }
}

} // namespace

extern "C" int32_t OotmmSouls_SuppressSpawn(PlayState* play, int16_t actorId, int16_t params) {
    if (play == nullptr || !OotmmSession_IsActive() || gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return 0;
    }

    const SpawnVerdict verdict = ResolveSpawn(play, actorId, static_cast<uint16_t>(params));
    if (verdict == SpawnVerdict::Allow) {
        return 0;
    }
    if (verdict == SpawnVerdict::BlockAndHoldRoom) {
        sRoomHidesEnemy = true;
    }
    ReplaceMissingActor(play, actorId);
    return 1;
}

extern "C" void OotmmSouls_ResetRoomState(void) {
    sRoomHidesEnemy = false;
}

extern "C" int32_t OotmmSouls_RoomClearBlocked(void) {
    return OotmmSession_IsActive() && sRoomHidesEnemy;
}

extern "C" int32_t OotmmSouls_Withheld(const char* soul) {
    if (soul == nullptr || !OotmmSession_IsActive()) {
        return 0;
    }
    return OwnsSoul(soul) ? 0 : 1;
}
