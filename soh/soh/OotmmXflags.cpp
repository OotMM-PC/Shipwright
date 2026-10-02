#include "OotmmXflags.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "ObjectExtension/ActorListIndex.h"
#include "ObjectExtension/ObjectExtension.h"
#include "OotmmCheckLedger.h"
#include "OotmmDrops.h"
#include "OotmmIpc.h"
#include "OotmmItemModels.h"
#include "OotmmSession.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SetAlternateHeaders.h"

#include <optional>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "overlays/actors/ovl_En_Wood02/z_en_wood02.h"

extern PlayState* gPlayState;
}

namespace {

struct Xflag {
    int Scene = 0;
    int Setup = 0;
    int Room = 0;
    int Slice = 0;
    int Id = 0;
};

struct XflagTag {
    Xflag Value;
};
ObjectExtension::Register<XflagTag> sXflagTagRegister;

struct FairyDraw {
    ActorFunc Original = nullptr;
};
ObjectExtension::Register<FairyDraw> sFairyDrawRegister;

int sLastScene = -1;

constexpr const char* kXflagTypes[] = { "pot",       "grass",  "crate", "barrel",     "hive",   "rupee",
                                        "heart",     "fairy",  "butterfly", "wonder", "rock",   "redboulder",
                                        "redice",    "icicle", "snowball",  "soil",   "tree",   "bush",
                                        "fairy_spot" };

int SceneSetup(PlayState* play) {
    const int requested = gSaveContext.sceneSetupIndex;
    if (requested <= 0 || requested > 3 || play->sceneSegment == nullptr) {
        return 0;
    }
    const auto* scene = static_cast<SOH::Scene*>(play->sceneSegment);
    for (const auto& command : scene->commands) {
        if (command == nullptr || command->cmdId != SOH::SceneCommandID::SetAlternateHeaders) {
            continue;
        }
        const auto* alternates = static_cast<SOH::SetAlternateHeaders*>(command.get());
        for (int setup = requested; setup > 0; --setup) {
            if (static_cast<size_t>(setup - 1) < alternates->headers.size() &&
                alternates->headers[setup - 1] != nullptr) {
                return setup;
            }
        }
        return 0;
    }
    return 0;
}

void AliasPot(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_KAKARIKO_VILLAGE:
        case SCENE_LON_LON_RANCH:
            if (xf.Setup == 1) {
                xf.Setup = 0;
                xf.Id += 1;
            }
            break;
        case SCENE_ZORAS_DOMAIN:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                switch (xf.Id) {
                    case 1: xf.Id = 25; break;
                    case 2: xf.Id = 22; break;
                    case 3: xf.Id = 24; break;
                    case 4: xf.Id = 21; break;
                    case 5: xf.Id = 23; break;
                }
            }
            break;
        case SCENE_ZORAS_FOUNTAIN:
            if (xf.Setup == 1) {
                xf.Setup = 0;
            }
            break;
        case SCENE_DEATH_MOUNTAIN_CRATER:
            if (xf.Setup == 0) {
                xf.Setup = 2;
                xf.Id -= 2;
            }
            break;
        case SCENE_GORON_CITY:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                switch (xf.Room) {
                    case 1: xf.Id += 4; break;
                    case 3: xf.Id += 33; break;
                }
            }
            break;
        case SCENE_GANONDORF_BOSS:
            xf.Scene = SCENE_GANONS_TOWER;
            xf.Room = 8;
            xf.Id -= 46;
            break;
        case SCENE_GANONS_TOWER_COLLAPSE_INTERIOR:
            xf.Scene = SCENE_GANONS_TOWER;
            xf.Room = 8;
            xf.Id -= 15;
            break;
    }
}

void AliasGrass(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_MARKET_NIGHT:
            xf.Scene = SCENE_MARKET_DAY;
            xf.Id += 16;
            break;
        case SCENE_KAKARIKO_VILLAGE:
            switch (xf.Setup) {
                case 1: xf.Id += 5; break;
                case 2: xf.Id += 12; break;
                case 3: xf.Id += 14; break;
            }
            xf.Setup = 0;
            break;
        case SCENE_ZORAS_RIVER:
            if (xf.Setup == 2) {
                xf.Id += 28;
                xf.Setup = 0;
            }
            break;
        case SCENE_KOKIRI_FOREST:
            if (xf.Setup == 3) {
                xf.Setup = 2;
                xf.Id -= 7;
            }
            if (xf.Setup == 0 && xf.Room == 2 && xf.Id == 9) {
                xf.Room = 0;
                xf.Id = 62;
            }
            break;
        case SCENE_LAKE_HYLIA:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id -= 8;
            }
            break;
        case SCENE_LOST_WOODS:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                switch (xf.Room) {
                    case 2: xf.Id -= 2; break;
                    case 7: xf.Id -= 1; break;
                    case 8: xf.Id -= 1; break;
                }
            }
            break;
    }
}

void AliasGrassRing(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_HYRULE_FIELD:
            switch (xf.Setup) {
                case 1: xf.Id += 2; break;
                case 2: xf.Id += 8; break;
            }
            xf.Setup = 0;
            break;
        case SCENE_GRAVEYARD:
            switch (xf.Setup) {
                case 1: xf.Id -= 4; break;
                case 2: xf.Id -= 13; break;
                case 3: xf.Id -= 13; break;
            }
            xf.Setup = 0;
            break;
        case SCENE_ZORAS_RIVER:
            if (xf.Setup == 2) {
                xf.Id += 28;
                xf.Setup = 0;
            }
            break;
        case SCENE_LAKE_HYLIA:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id -= 4;
            }
            break;
        case SCENE_KOKIRI_FOREST:
            if (xf.Setup == 3) {
                xf.Setup = 2;
                xf.Id -= 7;
            }
            break;
    }
}

void AliasRockRing(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_HYRULE_FIELD:
            xf.Setup = 0;
            xf.Id = 55;
            break;
        case SCENE_KOKIRI_FOREST:
            switch (xf.Setup) {
                case 2: xf.Id += 36; break;
                case 3: xf.Id += 33; break;
            }
            xf.Setup = 0;
            break;
        case SCENE_ZORAS_DOMAIN:
            xf.Setup = 0;
            xf.Id = 10;
            break;
        case SCENE_DESERT_COLOSSUS:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id -= 2;
            }
            break;
        case SCENE_ZORAS_RIVER:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                switch (xf.Id) {
                    case 21: xf.Id = 51; break;
                    case 22: xf.Id = 50; break;
                }
            }
            break;
        case SCENE_DEATH_MOUNTAIN_TRAIL:
            xf.Setup = 0;
            xf.Id = 33;
            break;
        case SCENE_DEATH_MOUNTAIN_CRATER:
            xf.Setup = 0;
            xf.Id = 32;
            break;
    }
}

void AliasRock(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_LAKE_HYLIA:
            xf.Setup = 0;
            xf.Id = 22;
            break;
        case SCENE_DESERT_COLOSSUS:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id -= 2;
            }
            break;
        case SCENE_GORON_CITY:
            xf.Setup = 0;
            xf.Id = 47;
            break;
        case SCENE_KAKARIKO_VILLAGE:
            switch (xf.Setup) {
                case 1: xf.Id += 5; break;
                case 2: xf.Id += 11; break;
                case 3: xf.Id += 12; break;
            }
            xf.Setup = 0;
            break;
        case SCENE_GRAVEYARD:
            xf.Setup = 0;
            xf.Id = 14;
            break;
        case SCENE_ZORAS_RIVER:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id += 29;
            }
            break;
        case SCENE_GERUDO_VALLEY:
            if (xf.Setup == 0) {
                xf.Setup = 2;
                switch (xf.Id) {
                    case 23: xf.Id = 42; break;
                    case 24: xf.Id = 43; break;
                    case 25: xf.Id = 33; break;
                    case 26: xf.Id = 34; break;
                    case 27: xf.Id = 35; break;
                    case 28: xf.Id = 44; break;
                }
            }
            break;
        case SCENE_DEATH_MOUNTAIN_TRAIL:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id -= 8;
            }
            break;
    }
}

bool AliasBush(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_HYRULE_FIELD:
            if (xf.Setup == 2) {
                xf.Setup = 1;
                switch (xf.Id) {
                    case 54: xf.Id = 38; break;
                    case 55: xf.Id = 39; break;
                    case 56: xf.Id = 40; break;
                    case 57: xf.Id = 45; break;
                    case 58: xf.Id = 46; break;
                    case 59: xf.Id = 43; break;
                    default: return false;
                }
            }
            break;
        case SCENE_ZORAS_FOUNTAIN:
            xf.Setup = 0;
            break;
    }
    return true;
}

bool AliasTree(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_MARKET_NIGHT:
            xf.Scene = SCENE_MARKET_DAY;
            xf.Id = 14;
            break;
        case SCENE_HYRULE_FIELD:
            if (xf.Setup == 2 && xf.Id != 66) {
                xf.Setup = 1;
                switch (xf.Id) {
                    case 38: xf.Id = 33; break;
                    case 39: xf.Id = 34; break;
                    case 40: xf.Id = 35; break;
                    case 41: xf.Id = 32; break;
                    case 42: xf.Id = 31; break;
                    case 43: xf.Id = 29; break;
                    case 44: xf.Id = 36; break;
                    case 45: xf.Id = 37; break;
                    case 46: xf.Id = 49; break;
                    case 60: xf.Id = 30; break;
                    case 61: xf.Id = 62; break;
                    case 62: xf.Id = 63; break;
                    case 63: xf.Id = 64; break;
                    case 64: xf.Id = 65; break;
                    case 65: xf.Id = 61; break;
                    default: return false;
                }
            }
            break;
    }
    return true;
}

void AliasLargeCrate(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_MARKET_NIGHT:
            xf.Scene = SCENE_MARKET_DAY;
            xf.Id += 11;
            break;
        case SCENE_LON_LON_RANCH:
            if (xf.Setup == 1) {
                xf.Setup = 0;
                xf.Id -= 5;
            }
            break;
        case SCENE_GRAVEYARD:
            switch (xf.Setup) {
                case 1: xf.Id -= 2; break;
                case 2:
                case 3: xf.Id -= 12; break;
            }
            xf.Setup = 0;
            break;
        case SCENE_GERUDO_VALLEY:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id += 8;
            }
            break;
        case SCENE_KAKARIKO_VILLAGE:
            if (xf.Setup == 1) {
                xf.Setup = 0;
                xf.Id += 7;
            } else if (xf.Setup == 3) {
                xf.Setup = 2;
                xf.Id += 2;
            }
            break;
        case SCENE_GERUDOS_FORTRESS:
            switch (xf.Room) {
                case 0x00:
                    if (xf.Setup == 0) {
                        switch (xf.Id) {
                            case 14: xf.Id = 12; break;
                            case 15: xf.Id = 13; break;
                            case 16: xf.Id = 14; break;
                            case 17: xf.Id = 15; break;
                            case 18: xf.Id = 11; break;
                            case 19: xf.Id = 16; break;
                            case 20: xf.Id = 17; break;
                            case 21: xf.Id = 18; break;
                        }
                    }
                    xf.Setup = 2;
                    break;
                case 0x01:
                    switch (xf.Setup) {
                        case 2: xf.Id -= 8; break;
                        case 3: xf.Id -= 1; break;
                    }
                    if (xf.Setup != 0) {
                        if (xf.Id >= 3) {
                            xf.Id += 1;
                        }
                        xf.Setup = 0;
                    }
                    break;
            }
            break;
    }
}

void AliasFreestandingRupee(Xflag& xf) {
    if (xf.Scene == SCENE_LOST_WOODS && xf.Room == 7) {
        xf.Setup = 0;
        xf.Id = 4;
    }
}

void AliasWonder(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_SACRED_FOREST_MEADOW:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                xf.Id -= 3;
            }
            break;
        case SCENE_HYRULE_FIELD:
            if (xf.Setup == 1) {
                xf.Setup = 0;
                xf.Id += 2;
            }
            break;
        case SCENE_LON_LON_RANCH:
            if (xf.Setup == 3) {
                xf.Setup = 2;
                xf.Id += 16;
            }
            break;
        case SCENE_GERUDOS_FORTRESS:
            if (xf.Setup != 0) {
                xf.Setup = 0;
                xf.Id = xf.Id == 5 ? 8 : 7;
            }
            break;
        case SCENE_DESERT_COLOSSUS:
            if (xf.Setup == 2) {
                xf.Setup = 0;
                switch (xf.Id) {
                    case 22: xf.Id = 19; break;
                    case 23: xf.Id = 21; break;
                    case 24: xf.Id = 20; break;
                    case 25: xf.Id = 23; break;
                }
            }
            break;
        case SCENE_KAKARIKO_VILLAGE:
            if (xf.Setup == 1) {
                xf.Setup = 0;
                xf.Id += 7;
            }
            break;
    }
}

void AliasBoulder(Xflag& xf) {
    if (xf.Scene == SCENE_GORON_CITY && xf.Setup == 2) {
        xf.Setup = 0;
        switch (xf.Id) {
            case 1: xf.Id = 41; break;
            case 2: xf.Id = 42; break;
            case 3: xf.Id = 43; break;
            case 4: xf.Id = 45; break;
            case 5: xf.Id = 46; break;
        }
    } else if (xf.Scene == SCENE_DEATH_MOUNTAIN_CRATER && xf.Setup == 2) {
        xf.Setup = 0;
        switch (xf.Id) {
            case 4: xf.Id = 12; break;
            case 5: xf.Id = 13; break;
            case 6: xf.Id = 11; break;
            case 7: xf.Id = 14; break;
        }
    }
}

void AliasSoil(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_KOKIRI_FOREST: xf.Setup = 0; xf.Id = 73; break;
        case SCENE_GRAVEYARD: xf.Setup = 0; xf.Id = 18; break;
        case SCENE_ZORAS_RIVER: xf.Setup = 0; xf.Id = 56; break;
        case SCENE_LOST_WOODS: xf.Setup = 0; xf.Id = xf.Room == 5 ? 3 : 5; break;
        case SCENE_DESERT_COLOSSUS: xf.Setup = 0; xf.Id = 29; break;
        case SCENE_DEATH_MOUNTAIN_TRAIL: xf.Setup = 0; xf.Id = 43; break;
        case SCENE_DEATH_MOUNTAIN_CRATER: xf.Setup = 0; xf.Id = 33; break;
        case SCENE_GERUDO_VALLEY:
        case SCENE_LAKE_HYLIA: xf.Setup = 0; break;
    }
}

void RemapScrubGrottoHive(Xflag& xf) {
    if (xf.Room == 0x09) {
        switch (sLastScene) {
            case SCENE_SACRED_FOREST_MEADOW: xf.Room = 0x21; break;
            case SCENE_ZORAS_RIVER: xf.Room = 0x24; break;
            case SCENE_GERUDO_VALLEY: xf.Room = 0x25; break;
            case SCENE_DESERT_COLOSSUS: xf.Room = 0x26; break;
        }
    } else if (xf.Room == 0x0C) {
        switch (sLastScene) {
            case SCENE_LON_LON_RANCH: xf.Room = 0x27; break;
            case SCENE_GORON_CITY: xf.Room = 0x2A; break;
            case SCENE_DEATH_MOUNTAIN_CRATER: xf.Room = 0x2B; break;
            case SCENE_LAKE_HYLIA: xf.Room = 0x2D; break;
        }
    }
}

bool AliasFairy(Xflag& xf) {
    if (xf.Scene == SCENE_FAIRYS_FOUNTAIN) {
        switch (sLastScene) {
            case SCENE_HYRULE_FIELD: xf.Room = 0x20; break;
            case SCENE_ZORAS_RIVER: xf.Room = 0x21; break;
            case SCENE_SACRED_FOREST_MEADOW: xf.Room = 0x22; break;
            case SCENE_ZORAS_DOMAIN: xf.Room = 0x23; break;
            case SCENE_GERUDOS_FORTRESS: xf.Room = 0x24; break;
            default: return false;
        }
    } else if (xf.Scene == SCENE_DESERT_COLOSSUS) {
        xf.Id = 1;
        xf.Setup = 0;
    }
    return true;
}

void AliasSongFairy(Xflag& xf) {
    switch (xf.Scene) {
        case SCENE_HYRULE_FIELD: xf.Setup = 0; xf.Id = 0x48; break;
        case SCENE_LAKE_HYLIA: xf.Setup = 0; xf.Id = 0x37; break;
        case SCENE_LOST_WOODS: xf.Setup = 0; xf.Id = 0x03; break;
        case SCENE_DEATH_MOUNTAIN_TRAIL: xf.Setup = 0; xf.Id = 0x26; break;
    }
}

bool IsTree(const Actor* actor) {
    return (actor->params & 0xFF) <= WOOD_TREE_KAKARIKO_ADULT;
}

bool IsFreestandingRupee(const Actor* actor) {
    const int type = actor->params & 0xFF;
    return type == ITEM00_RUPEE_GREEN || type == ITEM00_RUPEE_BLUE || type == ITEM00_RUPEE_RED;
}

bool Alias(const Actor* actor, Xflag& xf) {
    switch (actor->id) {
        case ACTOR_OBJ_TSUBO:
            AliasPot(xf);
            return true;
        case ACTOR_EN_KUSA:
            AliasGrass(xf);
            return true;
        case ACTOR_EN_ISHI:
            AliasRock(xf);
            return true;
        case ACTOR_OBJ_MURE2:
            if ((actor->params & 3) == 2) {
                AliasRockRing(xf);
            } else {
                AliasGrassRing(xf);
            }
            return true;
        case ACTOR_EN_WOOD02:
            return IsTree(actor) ? AliasTree(xf) : AliasBush(xf);
        case ACTOR_OBJ_KIBAKO2:
            AliasLargeCrate(xf);
            return true;
        case ACTOR_EN_ITEM00:
            if (IsFreestandingRupee(actor)) {
                AliasFreestandingRupee(xf);
            }
            return true;
        case ACTOR_EN_WONDER_ITEM:
            AliasWonder(xf);
            return true;
        case ACTOR_OBJ_MURE3:
            if (xf.Scene == SCENE_KOKIRI_FOREST) {
                xf.Setup = 2;
                xf.Id = 11;
            }
            return true;
        case ACTOR_OBJ_HAMISHI:
            AliasBoulder(xf);
            return true;
        case ACTOR_OBJ_BEAN:
            AliasSoil(xf);
            return true;
        case ACTOR_EN_ELF:
            return (actor->params & 0xF) >= FAIRY_HEAL_TIMED && AliasFairy(xf);
        case ACTOR_SHOT_SUN:
            if ((actor->params & 0xFF) != 0x40 && (actor->params & 0xFF) != 0x41) {
                return false;
            }
            AliasSongFairy(xf);
            return true;
        default:
            return true;
    }
}

bool RemapsGrotto(const Actor* actor) {
    switch (actor->id) {
        case ACTOR_OBJ_KIBAKO:
        case ACTOR_OBJ_KIBAKO2:
        case ACTOR_OBJ_HAMISHI:
        case ACTOR_BG_ICE_SHELTER:
            return false;
        default:
            return true;
    }
}

std::optional<Xflag> Resolve(Actor* actor) {
    if (actor == nullptr || gPlayState == nullptr) {
        return std::nullopt;
    }
    if (const auto* tag = ObjectExtension::GetInstance().Get<XflagTag>(actor)) {
        return tag->Value;
    }
    const int index = GetActorListIndex(actor);
    if (index < 0) {
        return std::nullopt;
    }
    Xflag xf{ gPlayState->sceneNum, SceneSetup(gPlayState), actor->room, 0, index };
    if (actor->id == ACTOR_OBJ_HAMISHI && xf.Scene == SCENE_GERUDO_VALLEY && xf.Setup == 0) {
        return std::nullopt;
    }
    if (RemapsGrotto(actor) && xf.Scene == SCENE_GROTTOS && xf.Room == 0) {
        xf.Room = 0x20 | (gSaveContext.respawn[RESPAWN_MODE_RETURN].data & 0x1F);
    }
    if (actor->id == ACTOR_OBJ_COMB && xf.Scene == SCENE_GROTTOS) {
        RemapScrubGrottoHive(xf);
    }
    if (!Alias(actor, xf)) {
        return std::nullopt;
    }
    return xf;
}

uint32_t NativeId(const Xflag& xf) {
    return (static_cast<uint32_t>(xf.Slice) << 16) | (static_cast<uint32_t>(xf.Setup & 3) << 14) |
           (static_cast<uint32_t>(xf.Room) << 8) | static_cast<uint32_t>(xf.Id & 0xFF);
}

const Ship::OotmmPlacement* Lookup(const Xflag& xf) {
    if (xf.Scene < 0 || xf.Id < 0 || !OotmmSession_IsActive()) {
        return nullptr;
    }
    const auto& state = OotmmSession_GetState();
    const uint32_t nativeId = NativeId(xf);
    for (const char* type : kXflagTypes) {
        if (const auto* placement =
                state.FindCheck(Ship::OotmmGame::Oot, static_cast<uint32_t>(xf.Scene), type, nativeId)) {
            return placement;
        }
    }
    return nullptr;
}

bool Collected(const Ship::OotmmPlacement& placement) {
    return OotmmCheckLedger_Contains(placement.CheckId) || OotmmIpc_IsCheckCompleted(placement.CheckId);
}

bool Excluded(const Actor* actor) {
    return actor->id == ACTOR_EN_WONDER_ITEM && gPlayState->sceneNum == SCENE_CASTLE_COURTYARD_ZELDA &&
           OotmmSession_GetState().GetBoolSetting("skipZelda", false);
}

const Ship::OotmmPlacement* Shuffled(Actor* actor) {
    const auto xf = Resolve(actor);
    if (!xf.has_value() || Excluded(actor)) {
        return nullptr;
    }
    const auto* placement = Lookup(*xf);
    return placement != nullptr && !Collected(*placement) ? placement : nullptr;
}

void Report(const Ship::OotmmPlacement& placement, bool present) {
    OotmmIpc_SendCheckCollected(placement.CheckId, present);
    OotmmCheckLedger_Record(placement.CheckId);
}

bool IsNothing(const Ship::OotmmPlacement& placement) {
    return placement.ItemId == "NOTHING";
}

void AdoptFreestanding(Actor* actor) {
    if (actor == nullptr || actor->id != ACTOR_EN_ITEM00) {
        return;
    }
    const int type = actor->params & 0xFF;
    if (!IsFreestandingRupee(actor) && type != ITEM00_HEART) {
        return;
    }
    const auto* placement = Shuffled(actor);
    if (placement == nullptr) {
        return;
    }
    auto* item = reinterpret_cast<EnItem00*>(actor);
    OotmmDrops_Adopt(actor, *placement, item->collectibleFlag);
}

const Ship::OotmmPlacement* Pending(const std::optional<Xflag>& xf) {
    if (!xf.has_value()) {
        return nullptr;
    }
    const auto* placement = Lookup(*xf);
    return placement != nullptr && !Collected(*placement) ? placement : nullptr;
}

bool DropPlacement(const Ship::OotmmPlacement* placement, PlayState* play, const Vec3f& pos, bool still) {
    if (placement == nullptr) {
        return false;
    }
    if (IsNothing(*placement)) {
        Report(*placement, false);
        return true;
    }
    if (OotmmDrops_HasCheck(placement->CheckId)) {
        return true;
    }
    Actor* drop = OotmmDrops_SpawnPlacement(play, pos.x, pos.y, pos.z, *placement);
    if (drop != nullptr && still) {
        drop->speedXZ = 0.0f;
        drop->velocity = Vec3f{ 0.0f, 0.0f, 0.0f };
    }
    return true;
}

Vec3f DropPosition(Actor* actor, const Xflag& xf) {
    if (actor->id == ACTOR_OBJ_HAMISHI && xf.Scene == SCENE_DEATH_MOUNTAIN_CRATER && xf.Id == 14 &&
        gPlayState != nullptr) {
        return GET_PLAYER(gPlayState)->actor.world.pos;
    }
    return actor->world.pos;
}

std::optional<Xflag> sPendingSpawn;

void DrawFairy(Actor* actor, PlayState* play) {
    const auto* placement = Shuffled(actor);
    if (placement == nullptr) {
        const auto* original = ObjectExtension::GetInstance().Get<FairyDraw>(actor);
        if (original != nullptr && original->Original != nullptr) {
            original->Original(actor, play);
        }
        return;
    }
    if (actor->scale.x > 0.0001f) {
        const float scale = 0.25f / actor->scale.x;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    }
    const bool cloaked = !placement->CloakItemId.empty();
    OotmmItemModel_DrawById(play, cloaked ? placement->CloakItemId : placement->ItemId,
                            cloaked ? placement->CloakItemName : placement->ItemName);
}

void AdoptFairy(Actor* actor) {
    if (actor == nullptr || actor->id != ACTOR_EN_ELF) {
        return;
    }
    const int type = actor->params & 0xF;
    if (type < FAIRY_HEAL_TIMED || type == FAIRY_SPAWNER || Shuffled(actor) == nullptr ||
        ObjectExtension::GetInstance().Has<FairyDraw>(actor)) {
        return;
    }
    ObjectExtension::GetInstance().Set<FairyDraw>(actor, FairyDraw{ actor->draw });
    actor->draw = DrawFairy;
}

int DropActor(Actor* actor, PlayState* play, bool still) {
    if (actor == nullptr || Excluded(actor)) {
        return 0;
    }
    const auto xf = Resolve(actor);
    if (!xf.has_value()) {
        return 0;
    }
    return DropPlacement(Pending(xf), play, DropPosition(actor, *xf), still);
}

} // namespace

extern "C" int OotmmXflags_IsShuffled(Actor* actor) {
    return Shuffled(actor) != nullptr;
}

extern "C" int OotmmXflags_Drop(Actor* actor, PlayState* play) {
    return DropActor(actor, play, false);
}

extern "C" int OotmmXflags_DropStill(Actor* actor, PlayState* play) {
    return DropActor(actor, play, true);
}

extern "C" int OotmmXflags_DropSlice(Actor* actor, PlayState* play, int slice, float x, float y, float z) {
    auto xf = Resolve(actor);
    if (!xf.has_value()) {
        return 0;
    }
    xf->Slice = slice;
    return DropPlacement(Pending(xf), play, Vec3f{ x, y, z }, false);
}

extern "C" int OotmmXflags_Collect(Actor* actor) {
    const auto* placement = Shuffled(actor);
    if (placement == nullptr) {
        return 0;
    }
    OotmmDrops_Report(placement->CheckId, placement->ItemId, !IsNothing(*placement));
    return 1;
}

extern "C" void OotmmXflags_TagChild(Actor* child, Actor* parent, int slice) {
    if (child == nullptr) {
        return;
    }
    const auto xf = Resolve(parent);
    if (!xf.has_value()) {
        return;
    }
    Xflag tagged = *xf;
    tagged.Slice = slice;
    ObjectExtension::GetInstance().Set<XflagTag>(child, XflagTag{ tagged });
    AdoptFreestanding(child);
}

extern "C" void OotmmXflags_PrepareSpawn(Actor* parent, int slice) {
    sPendingSpawn = Resolve(parent);
    if (sPendingSpawn.has_value()) {
        sPendingSpawn->Slice = slice;
    }
}

extern "C" void OotmmXflags_PrepareSpawnAt(int scene, int setup, int room, int id) {
    sPendingSpawn = Xflag{ scene, setup, room, 0, id };
}

extern "C" void OotmmXflags_ClearSpawn(void) {
    sPendingSpawn.reset();
}

extern "C" void OotmmXflags_ClaimSpawn(Actor* actor) {
    if (sPendingSpawn.has_value()) {
        ObjectExtension::GetInstance().Set<XflagTag>(actor, XflagTag{ *sPendingSpawn });
        sPendingSpawn.reset();
    }
}

extern "C" int OotmmXflags_PrepareFairy(Actor* butterfly) {
    const auto xf = Resolve(butterfly);
    sPendingSpawn = xf;
    const auto* placement = Pending(xf);
    if (placement != nullptr && IsNothing(*placement)) {
        Report(*placement, false);
        sPendingSpawn.reset();
        return 0;
    }
    return 1;
}

extern "C" int OotmmXflags_IsAnySliceShuffled(Actor* actor, int count) {
    const auto xf = Resolve(actor);
    if (!xf.has_value()) {
        return 0;
    }
    for (int slice = 0; slice < count; ++slice) {
        Xflag sliced = *xf;
        sliced.Slice = slice;
        const auto* placement = Lookup(sliced);
        if (placement != nullptr && !Collected(*placement)) {
            return 1;
        }
    }
    return 0;
}

int OotmmXflags_LastScene() {
    return sLastScene;
}

void OotmmXflags_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        if (sceneNum != SCENE_GROTTOS && sceneNum != SCENE_FAIRYS_FOUNTAIN) {
            sLastScene = sceneNum;
        }
    });
    REGISTER_VB_SHOULD(VB_POT_DROP_ITEM, {
        Actor* actor = va_arg(args, Actor*);
        if (OotmmXflags_Drop(actor, gPlayState)) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GRASS_DROP_ITEM, {
        Actor* actor = va_arg(args, Actor*);
        if (OotmmXflags_Drop(actor, gPlayState)) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_SMALL_CRATE_DROP_ITEM, {
        Actor* actor = va_arg(args, Actor*);
        if (OotmmXflags_Drop(actor, gPlayState)) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_CRATE_DROP_ITEM, {
        Actor* actor = va_arg(args, Actor*);
        if (OotmmXflags_Drop(actor, gPlayState)) {
            *should = false;
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](void* actor) {
        AdoptFreestanding(static_cast<Actor*>(actor));
        AdoptFairy(static_cast<Actor*>(actor));
    });
}
