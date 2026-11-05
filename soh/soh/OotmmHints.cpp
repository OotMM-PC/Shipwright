#include "OotmmHints.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"
#include "OotmmXflags.h"
#include "soh/util.h"

#include <cctype>
#include <string>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
void Message_FindMessage(PlayState* play, u16 textId);
s32 Object_Spawn(ObjectContext* objectCtx, s16 objectId);
}

namespace {

constexpr uint16_t kGossipText = 0x2053;
constexpr uint16_t kGossipTruthText = 0x2054;
constexpr uint16_t kAltarChildText = 0x7040;
constexpr uint16_t kAltarAdultText = 0x7088;
constexpr uint16_t kGanondorfText = 0x70CC;
constexpr uint16_t kNaviText = 0x0141;
constexpr uint16_t kSignStyleText = 0x0300;
constexpr int16_t kLocationSign = 0x1000;
constexpr int16_t kSpecialCondSign = 0x1100;
constexpr uint16_t kBridgeSignText = kSpecialCondSign | kSignStyleText;
constexpr char kFast = '\x08';
constexpr char kIcon = '\x13';
constexpr char kSignal = '\x0B';
constexpr char kEnd = '\x02';
constexpr char kAltarIcons[] = { 0x6c, 0x6d, 0x6e, 0x6b, 0x66, 0x67, 0x68, 0x69, 0x6a };
constexpr char kOcarinaIcon = 0x08;
constexpr char kBossKeyIcon = 0x74;

char ColorCode(char token) {
    switch (token) {
        case 'r': return 0x41;
        case 'g': return 0x42;
        case 'b': return 0x43;
        case 'c': return 0x44;
        case 'p': return 0x45;
        case 'y':
        case 'o': return 0x46;
        default: return 0x40;
    }
}

std::string Native(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '%' && i + 1 < text.size()) {
            out += '\x05';
            out += ColorCode(text[++i]);
        } else if (c == '\n') {
            out += '\x01';
        } else if (c == '\f') {
            out += '\x04';
            out += kFast;
        } else {
            out += c;
        }
    }
    return out;
}

std::string Box(const std::string& text) {
    return Native(Ship::OotmmHintLayout(text));
}

std::string Capitalized(std::string text) {
    if (!text.empty() && std::islower(static_cast<unsigned char>(text[0]))) {
        text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
    }
    return text;
}

void Load(uint16_t styleTextId, const std::string& raw, bool* loadFromMessageTable) {
    Message_FindMessage(gPlayState, styleTextId);
    Font* font = &gPlayState->msgCtx.font;
    gPlayState->msgCtx.msgLength = font->msgLength =
        SohUtils::CopyStringToCharBuffer(font->msgBuf, raw, sizeof(font->msgBuf));
    *loadFromMessageTable = false;
}

Actor* TalkActor(int16_t id) {
    Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
    if (player == nullptr || player->talkActor == nullptr || player->talkActor->id != id) {
        return nullptr;
    }
    return player->talkActor;
}

uint32_t GossipKey(const Actor* stone) {
    const uint32_t key = stone->params & 0x1F;
    if (key == 0x18) {
        return (gSaveContext.respawn[RESPAWN_MODE_RETURN].data & 0x1F) | 0x20;
    }
    return key;
}

void ShowSentence(uint16_t styleTextId, const std::string& text, bool* loadFromMessageTable) {
    Load(styleTextId, std::string(1, kFast) + Box(text) + kEnd, loadFromMessageTable);
}

void BuildGossipText(uint16_t* textId, bool* loadFromMessageTable) {
    Actor* stone = TalkActor(ACTOR_EN_GS);
    if (stone == nullptr || *textId != stone->textId) {
        return;
    }
    const std::string hint = OotmmSession_GetState().GetGossipHint(Ship::OotmmGame::Oot, GossipKey(stone));
    if (hint.empty()) {
        return;
    }
    stone->textId = kGossipText;
    ShowSentence(*textId, hint, loadFromMessageTable);
}

std::string RewardBox(char icon, const std::string& region) {
    return std::string(1, kFast) + kIcon + icon + Box(Capitalized(region) + "...");
}

void BuildAltarText(uint16_t* textId, bool* loadFromMessageTable) {
    if (TalkActor(ACTOR_EN_WONDER_TALK) == nullptr) {
        return;
    }
    const auto& state = OotmmSession_GetState();
    const auto& hints = state.GetHints();
    const bool child = *textId == kAltarChildText;
    const size_t base = child ? 0 : 3;
    const size_t count = child ? 3 : 6;
    if (hints.DungeonRewards.size() < base + count) {
        return;
    }
    std::string raw;
    for (size_t i = 0; i < count; ++i) {
        if (i != 0) {
            raw += '\x04';
        }
        raw += RewardBox(kAltarIcons[base + i], hints.DungeonRewards[base + i].Text);
    }
    const auto& ocarina = hints.StaticReward(Ship::OotmmStaticReward::OcarinaOfTime);
    const auto& song = hints.StaticReward(Ship::OotmmStaticReward::SongOfTime);
    if (child && !ocarina.empty() && !song.empty()) {
        raw += '\x04';
        raw += kIcon;
        raw += kOcarinaIcon;
        raw += kFast;
        raw += Box("It is also written that reuniting the %ySpiritual Stones %wleads to " + ocarina + " and " + song + ".");
    } else if (!child && state.GetStringSetting("ganonBossKey") == "anywhere") {
        raw += '\x04';
        raw += RewardBox(kBossKeyIcon, hints.GanonBossKey.Text);
    }
    raw += kSignal;
    raw += kEnd;
    Load(*textId, raw, loadFromMessageTable);
}

void BuildGanondorfText(uint16_t* textId, bool* loadFromMessageTable) {
    const auto& hints = OotmmSession_GetState().GetHints();
    if (hints.LightArrow.Text.empty()) {
        return;
    }
    ShowSentence(*textId, "Have you found the %yLight Arrows %w" + hints.LightArrow.Text + "?", loadFromMessageTable);
}

void BuildNaviText(uint16_t* textId, bool* loadFromMessageTable) {
    const auto& junk = OotmmSession_GetState().GetHints().Junk;
    if (junk.empty()) {
        return;
    }
    ShowSentence(*textId, junk[static_cast<uint16_t>(Rand_S16Offset(0, 0x7FFF)) % junk.size()], loadFromMessageTable);
}

const char* GenericGrottoName(int grotto) {
    switch (grotto) {
        case 0x00: return "Hyrule Field Market Generic Grotto";
        case 0x02: return "Hyrule Field Southeast Generic Grotto";
        case 0x03: return "Hyrule Field Open Generic Grotto";
        case 0x08: return "Kakariko Generic Grotto";
        case 0x09: return "Zora River Generic Grotto";
        case 0x0C: return "Kokiri Forest Generic Grotto";
        case 0x14: return "Lost Woods Generic Grotto";
        case 0x17: return "Death Mountain Trail Generic Grotto";
        case 0x1A: return "Death Mountain Crater Generic Grotto";
        default: return nullptr;
    }
}

const char* ScrubGrottoName(int room, int lastScene) {
    if (room == 0x09) {
        switch (lastScene) {
            case SCENE_SACRED_FOREST_MEADOW: return "Sacred Forest Meadow Scrub Grotto";
            case SCENE_ZORAS_RIVER: return "Zora River Scrub Grotto";
            case SCENE_GERUDO_VALLEY: return "Gerudo Valley Scrub Grotto";
            case SCENE_DESERT_COLOSSUS: return "Desert Colossus Scrub Grotto";
            default: return nullptr;
        }
    }
    switch (lastScene) {
        case SCENE_LON_LON_RANCH: return "Lon Lon Ranch Scrub Grotto";
        case SCENE_GORON_CITY: return "Goron City Scrub Grotto";
        case SCENE_DEATH_MOUNTAIN_CRATER: return "Death Mountain Crater Scrub Grotto";
        case SCENE_LAKE_HYLIA: return "Lake Hylia Scrub Grotto";
        default: return nullptr;
    }
}

const char* FairyFountainName(int lastScene) {
    switch (lastScene) {
        case SCENE_SACRED_FOREST_MEADOW: return "Sacred Forest Meadow Fairy Fountain";
        case SCENE_HYRULE_FIELD: return "Hyrule Field Fairy Fountain";
        case SCENE_ZORAS_RIVER: return "Zora River Fairy Fountain";
        case SCENE_ZORAS_DOMAIN: return "Zora Domain Fairy Fountain";
        case SCENE_GERUDOS_FORTRESS: return "Gerudo Fortress Fairy Fountain";
        default: return nullptr;
    }
}

const char* GreatFairyName(int scene, int spawn) {
    if (scene == SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC) {
        switch (spawn) {
            case 0: return "Death Mountain Trail Great Fairy Fountain";
            case 1: return "Death Mountain Crater Great Fairy Fountain";
            case 2: return "Outside Ganon Castle Great Fairy Fountain";
            default: return nullptr;
        }
    }
    switch (spawn) {
        case 0: return "Zora Fountain Great Fairy Fountain";
        case 1: return "Hyrule Castle Great Fairy Fountain";
        case 2: return "Desert Colossus Great Fairy Fountain";
        default: return nullptr;
    }
}

std::string LocationName() {
    const char* name = nullptr;
    const int lastScene = OotmmXflags_LastScene();
    switch (gPlayState->sceneNum) {
        case SCENE_GROTTOS:
            switch (gPlayState->roomCtx.curRoom.num) {
                case 0x00:
                    name = GenericGrottoName(gSaveContext.respawn[RESPAWN_MODE_RETURN].data & 0x1F);
                    break;
                case 0x06:
                    name = "Lost Woods Scrub Grotto";
                    break;
                case 0x09:
                case 0x0C:
                    name = ScrubGrottoName(gPlayState->roomCtx.curRoom.num, lastScene);
                    break;
            }
            break;
        case SCENE_FAIRYS_FOUNTAIN:
            name = FairyFountainName(lastScene);
            break;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC:
        case SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS:
            name = GreatFairyName(gPlayState->sceneNum, gPlayState->curSpawn);
            break;
    }
    return name != nullptr ? name : "Unknown Location";
}

void BuildSignText(uint16_t*, bool* loadFromMessageTable) {
    const Actor* sign = TalkActor(ACTOR_EN_KANBAN);
    if (sign == nullptr) {
        return;
    }
    if (sign->params == kLocationSign) {
        ShowSentence(kSignStyleText, LocationName(), loadFromMessageTable);
        return;
    }
    if (sign->params != kSpecialCondSign) {
        return;
    }
    const auto& conds = OotmmSession_GetState().GetHints().SpecialConds;
    if (const auto it = conds.find("BRIDGE"); it != conds.end()) {
        ShowSentence(kSignStyleText, it->second, loadFromMessageTable);
    }
}

void SpawnSign(float x, float y, float z, int16_t yaw, int16_t params) {
    if (Object_GetIndex(&gPlayState->objectCtx, OBJECT_KANBAN) < 0) {
        Object_Spawn(&gPlayState->objectCtx, OBJECT_KANBAN);
    }
    Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_KANBAN, x, y, z, 0, yaw, 0, params);
}

void SpawnLocationSigns() {
    const auto& state = OotmmSession_GetState();
    const bool grottos = state.GetStringSetting("erGrottos", "none") != "none";
    const bool indoors = state.GetStringSetting("erIndoors", "none") != "none";
    switch (gPlayState->sceneNum) {
        case SCENE_GROTTOS:
            if (grottos) {
                switch (gPlayState->roomCtx.curRoom.num) {
                    case 0x00: SpawnSign(-50.0f, 0.0f, 70.0f, 0x2000, kLocationSign); break;
                    case 0x06: SpawnSign(530.0f, 0.0f, 1260.0f, 0x2000, kLocationSign); break;
                    case 0x09: SpawnSign(2350.0f, 0.0f, 1260.0f, 0x2000, kLocationSign); break;
                    case 0x0C: SpawnSign(4700.0f, 0.0f, 1260.0f, 0x2000, kLocationSign); break;
                }
            }
            break;
        case SCENE_FAIRYS_FOUNTAIN:
            if (grottos) {
                SpawnSign(-90.0f, 38.0f, 250.0f, 0x2000, kLocationSign);
            }
            break;
        case SCENE_GREAT_FAIRYS_FOUNTAIN_MAGIC:
        case SCENE_GREAT_FAIRYS_FOUNTAIN_SPELLS:
            if (indoors) {
                SpawnSign(-90.0f, 20.0f, -150.0f, 0x2000, kLocationSign);
            }
            break;
    }
}

void SpawnExtraSigns() {
    if (gPlayState == nullptr || !OotmmSession_IsActive()) {
        return;
    }
    SpawnLocationSigns();
    const auto& state = OotmmSession_GetState();
    if (gPlayState->sceneNum != SCENE_OUTSIDE_GANONS_CASTLE || state.GetStringSetting("rainbowBridge") != "custom" ||
        !state.GetHints().SpecialConds.contains("BRIDGE")) {
        return;
    }
    SpawnSign(0.0f, 1358.0f, 1530.0f, 0x1000, kSpecialCondSign);
    SpawnSign(-600.0f, 1645.0f, 700.0f, 0x4800, kSpecialCondSign);
}

} // namespace

void OotmmHints_Init() {
    auto* interactor = GameInteractor::Instance;
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kGossipText, BuildGossipText);
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kGossipTruthText, BuildGossipText);
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kAltarChildText, BuildAltarText);
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kAltarAdultText, BuildAltarText);
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kGanondorfText, BuildGanondorfText);
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kNaviText, BuildNaviText);
    interactor->RegisterGameHookForID<GameInteractor::OnOpenText>(kBridgeSignText, BuildSignText);
    interactor->RegisterGameHook<GameInteractor::OnSceneSpawnActors>(SpawnExtraSigns);
}
