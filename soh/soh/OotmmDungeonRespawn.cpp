#include "OotmmDungeonRespawn.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmSession.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
}

namespace {

struct Record {
    std::string Game;
    uint16_t Entrance = 0;
    uint8_t Room = 0;
    float Pos[3] = {};
    int16_t Yaw = 0;
    int16_t Params = 0;
    int8_t Data = 0;
    uint32_t TempSwch = 0;
    uint32_t TempCollect = 0;
};

struct Store {
    std::optional<Record> Current;
    bool Pending = false;
    std::optional<Record> FwChild;
    std::optional<Record> FwAdult;
};

Store sStore;
bool sLoaded = false;
bool sFirstLoad = true;

std::filesystem::path StorePath() {
    return std::filesystem::u8path(OotmmSession_GetState().GetBootConfig().StatePath + ".dungeon-respawn.json");
}

nlohmann::json ToJson(const Record& record) {
    return { { "game", record.Game },         { "entrance", record.Entrance },
             { "room", record.Room },         { "pos", { record.Pos[0], record.Pos[1], record.Pos[2] } },
             { "yaw", record.Yaw },           { "params", record.Params },
             { "data", record.Data },         { "tempSwch", record.TempSwch },
             { "tempCollect", record.TempCollect } };
}

std::optional<Record> FromJson(const nlohmann::json& value) {
    if (!value.is_object() || !value.contains("game") || !value["game"].is_string() || !value.contains("pos") ||
        !value["pos"].is_array() || value["pos"].size() != 3) {
        return std::nullopt;
    }
    Record record;
    record.Game = value["game"].get<std::string>();
    record.Entrance = value.value("entrance", uint16_t{ 0 });
    record.Room = value.value("room", uint8_t{ 0 });
    for (size_t i = 0; i < 3; i++) {
        record.Pos[i] = value["pos"][i].is_number() ? value["pos"][i].get<float>() : 0.0f;
    }
    record.Yaw = value.value("yaw", int16_t{ 0 });
    record.Params = value.value("params", int16_t{ 0 });
    record.Data = value.value("data", int8_t{ 0 });
    record.TempSwch = value.value("tempSwch", uint32_t{ 0 });
    record.TempCollect = value.value("tempCollect", uint32_t{ 0 });
    return record;
}

void Load() {
    sStore = {};
    sLoaded = true;
    std::ifstream in(StorePath());
    if (!in) {
        return;
    }
    const auto root = nlohmann::json::parse(in, nullptr, false);
    if (!root.is_object()) {
        return;
    }
    if (root.contains("current")) {
        sStore.Current = FromJson(root["current"]);
    }
    sStore.Pending = sStore.Current.has_value() && root.value("pending", false);
    if (root.contains("fwOotChild")) {
        sStore.FwChild = FromJson(root["fwOotChild"]);
    }
    if (root.contains("fwOotAdult")) {
        sStore.FwAdult = FromJson(root["fwOotAdult"]);
    }
}

void Persist() {
    nlohmann::json root = nlohmann::json::object();
    std::ifstream in(StorePath());
    if (in) {
        auto existing = nlohmann::json::parse(in, nullptr, false);
        if (existing.is_object()) {
            root = std::move(existing);
        }
    }
    in.close();
    root["current"] = sStore.Current ? ToJson(*sStore.Current) : nlohmann::json(nullptr);
    root["pending"] = sStore.Pending;
    root["fwOotChild"] = sStore.FwChild ? ToJson(*sStore.FwChild) : nlohmann::json(nullptr);
    root["fwOotAdult"] = sStore.FwAdult ? ToJson(*sStore.FwAdult) : nlohmann::json(nullptr);
    const auto path = StorePath();
    const auto temporary = std::filesystem::path(path).concat(".tmp");
    {
        std::ofstream out(temporary, std::ios::trunc);
        out << root.dump();
    }
    std::error_code ec;
    std::filesystem::rename(temporary, path, ec);
}

Store& Ensure() {
    if (!sLoaded) {
        Load();
    }
    return sStore;
}

bool IsDungeonScene(int16_t scene) {
    switch (scene) {
        case SCENE_DEKU_TREE:
        case SCENE_DODONGOS_CAVERN:
        case SCENE_JABU_JABU:
        case SCENE_FOREST_TEMPLE:
        case SCENE_FIRE_TEMPLE:
        case SCENE_WATER_TEMPLE:
        case SCENE_SPIRIT_TEMPLE:
        case SCENE_SHADOW_TEMPLE:
        case SCENE_BOTTOM_OF_THE_WELL:
        case SCENE_ICE_CAVERN:
        case SCENE_GANONS_TOWER:
        case SCENE_GERUDO_TRAINING_GROUND:
        case SCENE_INSIDE_GANONS_CASTLE:
        case SCENE_DEKU_TREE_BOSS:
        case SCENE_DODONGOS_CAVERN_BOSS:
        case SCENE_JABU_JABU_BOSS:
        case SCENE_FOREST_TEMPLE_BOSS:
        case SCENE_FIRE_TEMPLE_BOSS:
        case SCENE_WATER_TEMPLE_BOSS:
        case SCENE_SPIRIT_TEMPLE_BOSS:
        case SCENE_SHADOW_TEMPLE_BOSS:
        case SCENE_GANONDORF_BOSS:
        case SCENE_GANON_BOSS:
            return true;
        default:
            return false;
    }
}

bool IsDungeonEntrance(uint16_t entrance) {
    switch (entrance) {
        case ENTR_DEKU_TREE_ENTRANCE:
        case ENTR_DODONGOS_CAVERN_ENTRANCE:
        case ENTR_JABU_JABU_ENTRANCE:
        case ENTR_FOREST_TEMPLE_ENTRANCE:
        case ENTR_FIRE_TEMPLE_ENTRANCE:
        case ENTR_WATER_TEMPLE_ENTRANCE:
        case ENTR_SHADOW_TEMPLE_ENTRANCE:
        case ENTR_SPIRIT_TEMPLE_ENTRANCE:
        case ENTR_BOTTOM_OF_THE_WELL_ENTRANCE:
        case ENTR_ICE_CAVERN_ENTRANCE:
        case ENTR_GERUDO_TRAINING_GROUND_ENTRANCE:
        case ENTR_INSIDE_GANONS_CASTLE_ENTRANCE:
        case ENTR_GANONS_TOWER_0:
            return true;
        default:
            return false;
    }
}

Record FromRespawn(const RespawnData& respawn) {
    Record record;
    record.Game = "oot";
    record.Entrance = static_cast<uint16_t>(respawn.entranceIndex);
    record.Room = respawn.roomIndex;
    record.Pos[0] = respawn.pos.x;
    record.Pos[1] = respawn.pos.y;
    record.Pos[2] = respawn.pos.z;
    record.Yaw = respawn.yaw;
    record.Params = respawn.playerParams;
    record.Data = respawn.data;
    record.TempSwch = respawn.tempSwchFlags;
    record.TempCollect = respawn.tempCollectFlags;
    if (record.Entrance == ENTR_SPIRIT_TEMPLE_2 || record.Entrance == ENTR_SPIRIT_TEMPLE_3) {
        record.Entrance = ENTR_SPIRIT_TEMPLE_ENTRANCE;
        record.Room = 0;
        record.Pos[0] = -5.0f;
        record.Pos[1] = -150.0f;
        record.Pos[2] = 754.0f;
        record.Yaw = static_cast<int16_t>(0x8000);
        record.Params = 0xDFF;
    }
    return record;
}

RespawnData ToRespawn(const Record& record) {
    RespawnData respawn{};
    respawn.pos.x = record.Pos[0];
    respawn.pos.y = record.Pos[1];
    respawn.pos.z = record.Pos[2];
    respawn.yaw = record.Yaw;
    respawn.playerParams = record.Params;
    respawn.entranceIndex = static_cast<s16>(record.Entrance);
    respawn.roomIndex = record.Room;
    respawn.data = record.Data;
    respawn.tempSwchFlags = record.TempSwch;
    respawn.tempCollectFlags = record.TempCollect;
    return respawn;
}

void ApplyLocal(PlayState* play, const Record& record) {
    const RespawnData respawn = ToRespawn(record);
    gSaveContext.respawn[RESPAWN_MODE_RETURN] = respawn;
    gSaveContext.respawn[RESPAWN_MODE_DOWN] = respawn;
    gSaveContext.respawnFlag = 2;
    if (play != nullptr) {
        play->nextEntranceIndex = respawn.entranceIndex;
    }
}

bool BeginForeign() {
    auto& store = Ensure();
    store.Pending = true;
    Persist();
    Ship::OotmmEntranceMapping mapping;
    mapping.FromGame = Ship::OotmmGame::Oot;
    mapping.From = "OoT dungeon entrance return";
    mapping.ToGame = Ship::OotmmGame::Mm;
    mapping.To = "Dungeon Entrance";
    mapping.ToNativeId = OOTMM_ENTR_DUNGEON_RESPAWN;
    return OotmmSession_BeginCrossGameTransition(mapping);
}

void OnSceneReady() {
    if (!OotmmSession_IsActive() || gPlayState == nullptr) {
        return;
    }
    auto& store = Ensure();
    if (IsDungeonScene(gPlayState->sceneNum)) {
        const Record arrival = FromRespawn(gSaveContext.respawn[RESPAWN_MODE_DOWN]);
        if (!store.Current.has_value() ||
            (IsDungeonEntrance(arrival.Entrance) &&
             (store.Current->Game != arrival.Game || store.Current->Entrance != arrival.Entrance))) {
            store.Current = arrival;
            store.Pending = false;
            Persist();
        }
        return;
    }
    if (store.Current.has_value() || store.Pending) {
        store.Current.reset();
        store.Pending = false;
        Persist();
    }
}

void OnLoadGame() {
    if (!OotmmSession_IsActive()) {
        return;
    }
    const bool transition = sFirstLoad && OotmmSession_GetState().GetBootConfig().BootEntrance.has_value();
    sFirstLoad = false;
    Load();
    if (!transition && (sStore.Current.has_value() || sStore.Pending)) {
        sStore.Current.reset();
        sStore.Pending = false;
        Persist();
    }
}

} // namespace

void OotmmDungeonRespawn_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) { OnLoadGame(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneSpawnActors>([]() { OnSceneReady(); });
}

extern "C" int32_t OotmmDungeonRespawn_Available(void) {
    return OotmmSession_IsActive() && Ensure().Current.has_value() ? 1 : 0;
}

extern "C" int32_t OotmmDungeonRespawn_Warp(PlayState* play) {
    if (!OotmmDungeonRespawn_Available() || play == nullptr) {
        return 0;
    }
    const Record record = *sStore.Current;
    if (record.Game != "oot") {
        return BeginForeign() ? 1 : 0;
    }
    ApplyLocal(play, record);
    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_FADE_BLACK;
    return 1;
}

extern "C" int32_t OotmmDungeonRespawn_RedirectRespawn(PlayState* play) {
    if (!OotmmDungeonRespawn_Available() || play == nullptr) {
        return 0;
    }
    const Record record = *sStore.Current;
    if (record.Game != "oot") {
        return BeginForeign() ? 1 : 0;
    }
    ApplyLocal(play, record);
    return 1;
}

extern "C" int32_t OotmmDungeonRespawn_ApplyArrival(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    auto& store = Ensure();
    if (!store.Current.has_value() || store.Current->Game != "oot") {
        return 0;
    }
    store.Pending = false;
    Persist();
    ApplyLocal(nullptr, *store.Current);
    gSaveContext.entranceIndex = static_cast<s16>(store.Current->Entrance);
    return 1;
}

extern "C" void OotmmDungeonRespawn_Clear(void) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    auto& store = Ensure();
    if (store.Current.has_value() || store.Pending) {
        store.Current.reset();
        store.Pending = false;
        Persist();
    }
}

extern "C" void OotmmDungeonRespawn_SaveForFaroresWind(void) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    auto& store = Ensure();
    (LINK_IS_ADULT ? store.FwAdult : store.FwChild) = store.Current;
    Persist();
}

extern "C" void OotmmDungeonRespawn_RestoreForFaroresWind(void) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    auto& store = Ensure();
    store.Current = LINK_IS_ADULT ? store.FwAdult : store.FwChild;
    store.Pending = false;
    Persist();
}
