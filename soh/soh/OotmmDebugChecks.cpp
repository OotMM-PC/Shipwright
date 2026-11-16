#include "OotmmDebugChecks.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmChecks.h"
#include "OotmmCsmc.h"
#include "OotmmCustomItems.h"
#include "OotmmDialog.h"
#include "OotmmDrops.h"
#include "OotmmNpcChecks.h"
#include "OotmmPauseInfo.h"
#include "OotmmBottles.h"
#include "OotmmPresence.h"
#include "OotmmIpc.h"
#include "OotmmItemApply.h"
#include "OotmmItemPresentation.h"
#include <libultraship/bridge/OotmmRuntimeTest.h>
#include "OotmmSession.h"
#include "OotmmTriforce.h"
#include "OotmmSongs.h"
#include "OotmmXflags.h"
#include "SaveManager.h"
#include "soh/SohGui/UIWidgets.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
void Audio_OcaGetPlaybackDebug(u8* state, u8* note, u8* pos);
#include "src/overlays/actors/ovl_En_Box/z_en_box.h"
#include "src/overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "src/overlays/actors/ovl_Boss_Dodongo/z_boss_dodongo.h"
#include "OotmmBomMm.h"
#include "OotmmClearTag.h"
#include "src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"
#include "src/overlays/actors/ovl_En_Si/z_en_si.h"
#include "src/overlays/actors/ovl_En_Syateki_Itm/z_en_syateki_itm.h"
#include "src/overlays/actors/ovl_En_Diving_Game/z_en_diving_game.h"
#include "src/overlays/actors/ovl_En_Dnt_Nomal/z_en_dnt_nomal.h"
#include "src/overlays/actors/ovl_En_Mk/z_en_mk.h"
#include "src/overlays/actors/ovl_En_Go2/z_en_go2.h"
void Fishing_DebugState(s32* out);
#if !defined(NDEBUG) || defined(OOTMM_RUNTIME_TEST)
#include "src/overlays/actors/ovl_Shot_Sun/z_shot_sun.h"
#include "src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.h"
#endif

extern PlayState* gPlayState;
typedef struct {
    u16 sfxId;
    Vec3f* pos;
    u8 token;
    f32* freqScale;
    f32* vol;
    s8* reverbAdd;
} OotmmDebugSoundRequest;
extern OotmmDebugSoundRequest sSoundRequests[0x100];
extern u8 sSoundRequestWriteIndex;
void Player_Action_8084E6D4(Player* player, PlayState* play);
void Play_SetRespawnData(PlayState* play, s32 respawnMode, s16 entranceIndex, s32 roomIndex, s32 playerParams,
                         Vec3f* pos, s16 yaw);
}

namespace {
#if !defined(NDEBUG) || defined(OOTMM_RUNTIME_TEST)
std::atomic<uint64_t> sRuntimePad{0};
#endif

constexpr int32_t kChestItem = 1;

struct AppearanceOption {
    const char* Label;
    bool HasClass;
    Ship::OotmmCsmcClass Class;
};

const AppearanceOption kAppearances[] = {
    { "Vanilla", false, Ship::OotmmCsmcClass::Normal },
    { "Normal (junk)", true, Ship::OotmmCsmcClass::Normal },
    { "Major", true, Ship::OotmmCsmcClass::Major },
    { "Boss Key", true, Ship::OotmmCsmcClass::BossKey },
    { "Small Key", true, Ship::OotmmCsmcClass::Key },
    { "Skulltula Token", true, Ship::OotmmCsmcClass::Spider },
    { "Stray Fairy", true, Ship::OotmmCsmcClass::Fairy },
    { "Heart", true, Ship::OotmmCsmcClass::Heart },
    { "Soul", true, Ship::OotmmCsmcClass::Soul },
    { "Map/Compass", true, Ship::OotmmCsmcClass::MapCompass },
};

s32 sDebugPoly = -1;
int sAppearance = 2;

std::unordered_map<Actor*, int32_t> sDebugChests;

bool IsDebugChest(Actor* actor) {
    return sDebugChests.count(actor) != 0;
}

int32_t FreeChestFlag() {
    for (int32_t flag = 31; flag >= 0; flag--) {
        if (Flags_GetTreasure(gPlayState, flag) != 0) {
            continue;
        }
        if (OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, gPlayState->sceneNum, "chest",
                                              static_cast<uint32_t>(flag)) != nullptr) {
            continue;
        }
        bool used = false;
        for (const auto& [chest, usedFlag] : sDebugChests) {
            if (usedFlag == flag) {
                used = true;
                break;
            }
        }
        if (!used) {
            return flag;
        }
    }
    return -1;
}

bool PlaceChest() {
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return false;
    }
    const int32_t flag = FreeChestFlag();
    if (flag < 0) {
        return false;
    }

    Vec3f pos = player->actor.world.pos;
    pos.x += Math_SinS(player->actor.shape.rot.y) * 80.0f;
    pos.z += Math_CosS(player->actor.shape.rot.y) * 80.0f;
    const s16 params = static_cast<s16>((kChestItem << 5) | flag);
    Actor* chest = Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_BOX, pos.x, pos.y, pos.z,
                               0, player->actor.shape.rot.y, 0, params);
    if (chest == nullptr) {
        return false;
    }

    sDebugChests.emplace(chest, flag);
    const auto& appearance = kAppearances[sAppearance];
    if (appearance.HasClass) {
        OotmmCsmc_SetChestOverride(reinterpret_cast<EnBox*>(chest), appearance.Class);
    }
    return true;
}

struct DropOption {
    std::string ItemId;
    std::string ItemName;
    std::string ItemGame;
};

std::vector<DropOption> sDropOptions;
int sDropSelection = 0;
char sDropFilter[64] = {};

void BuildDropOptions() {
    if (!sDropOptions.empty()) {
        return;
    }
    std::unordered_set<std::string> seen;
    for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
        if (placement.ItemId.empty() || !seen.insert(placement.ItemId).second) {
            continue;
        }
        sDropOptions.push_back({ placement.ItemId, placement.ItemName, placement.ItemGame });
    }
    std::sort(sDropOptions.begin(), sDropOptions.end(),
              [](const DropOption& a, const DropOption& b) { return a.ItemName < b.ItemName; });
}

bool MatchesFilter(const std::string& name, const char* filter) {
    if (filter[0] == '\0') {
        return true;
    }
    std::string lowerName = name;
    std::string lowerFilter = filter;
    const auto lower = [](unsigned char character) { return static_cast<char>(std::tolower(character)); };
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), lower);
    std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), lower);
    return lowerName.find(lowerFilter) != std::string::npos;
}

bool DropSelectedItem() {
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr || sDropSelection >= static_cast<int>(sDropOptions.size())) {
        return false;
    }
    const DropOption& option = sDropOptions[sDropSelection];
    const float x = player->actor.world.pos.x + Math_SinS(player->actor.shape.rot.y) * 60.0f;
    const float z = player->actor.world.pos.z + Math_CosS(player->actor.shape.rot.y) * 60.0f;
    return OotmmDrops_Spawn(gPlayState, x, player->actor.world.pos.y + 20.0f, z, option.ItemId,
                            option.ItemName, option.ItemGame, "");
}

void DrawDropSection() {
    ImGui::SeparatorText("Dropped items");
    ImGui::TextWrapped("Drops a collectible that draws as the item and presents it on pickup. "
                       "Grants nothing.");
    BuildDropOptions();
    if (sDropOptions.empty()) {
        ImGui::TextDisabled("No seed items available.");
        return;
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    ImGui::InputTextWithHint("##DropFilter", "Filter", sDropFilter, sizeof(sDropFilter));
    if (sDropSelection >= static_cast<int>(sDropOptions.size())) {
        sDropSelection = 0;
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    if (ImGui::BeginCombo("Item", sDropOptions[sDropSelection].ItemName.c_str())) {
        for (int i = 0; i < static_cast<int>(sDropOptions.size()); i++) {
            if (!MatchesFilter(sDropOptions[i].ItemName, sDropFilter)) {
                continue;
            }
            if (ImGui::Selectable(sDropOptions[i].ItemName.c_str(), i == sDropSelection)) {
                sDropSelection = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(gPlayState == nullptr);
    if (ImGui::Button("Drop item")) {
        DropSelectedItem();
    }
    ImGui::EndDisabled();
}

struct NpcOption {
    std::string NativeId;
    std::string Label;
    std::string ItemLabel;
    int32_t Entrance = -1;
    const char* RouteHint = nullptr;
};

std::vector<NpcOption> sNpcOptions;
int sNpcSelection = 0;
char sNpcFilter[64] = {};

constexpr int32_t kDekuTheaterEntrance = 0x05C4;

int32_t EntranceForScene(int32_t scene) {
    int32_t fallback = -1;
    for (int32_t entrance = 0; scene >= 0 && entrance < ENTR_MAX; entrance++) {
        if (gEntranceTable[entrance].scene != scene) {
            continue;
        }
        if (gEntranceTable[entrance].spawn == 0) {
            return entrance;
        }
        if (fallback < 0) {
            fallback = entrance;
        }
    }
    return fallback;
}

int32_t EntranceForNpc(const std::string& nativeId, int32_t scene) {
    if (nativeId == "OOT_THEATER_STICKS" || nativeId == "OOT_THEATER_NUTS") {
        return kDekuTheaterEntrance;
    }
    if (nativeId == "OOT_WEIRD_EGG" || nativeId == "OOT_POCKET_EGG") {
        return -1;
    }
    return EntranceForScene(scene);
}

const char* RouteHintForNpc(const std::string& nativeId) {
    if (nativeId == "OOT_WEIRD_EGG" || nativeId == "OOT_POCKET_EGG") {
        return "This check triggers by holding the egg until it hatches.";
    }
    return nullptr;
}

void BuildNpcOptions() {
    if (!sNpcOptions.empty()) {
        return;
    }
    for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
        if (placement.CheckGame != Ship::OotmmGame::Oot || placement.CheckType != "npc" ||
            placement.CheckNativeId.empty()) {
            continue;
        }
        const int32_t scene = placement.CheckScene.has_value() ? static_cast<int32_t>(*placement.CheckScene) : -1;
        sNpcOptions.push_back({ placement.CheckNativeId,
                                placement.CheckName.empty() ? placement.CheckNativeId : placement.CheckName,
                                placement.ItemName.empty() ? placement.ItemId : placement.ItemName,
                                EntranceForNpc(placement.CheckNativeId, scene),
                                RouteHintForNpc(placement.CheckNativeId) });
    }
    std::sort(sNpcOptions.begin(), sNpcOptions.end(),
              [](const NpcOption& a, const NpcOption& b) { return a.Label < b.Label; });
}

void DrawNpcSection() {
    ImGui::SeparatorText("NPC interactions");
    ImGui::TextWrapped("Go loads the NPC's scene. Complete the interaction in-game so the actor owns "
                       "the grant, dialogue, and state transition.");
    BuildNpcOptions();
    if (sNpcOptions.empty()) {
        ImGui::TextDisabled("No npc checks in the seed.");
        return;
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    ImGui::InputTextWithHint("##NpcFilter", "Filter", sNpcFilter, sizeof(sNpcFilter));
    if (sNpcSelection >= static_cast<int>(sNpcOptions.size())) {
        sNpcSelection = 0;
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    if (ImGui::BeginCombo("Check", sNpcOptions[sNpcSelection].Label.c_str())) {
        for (int i = 0; i < static_cast<int>(sNpcOptions.size()); i++) {
            if (!MatchesFilter(sNpcOptions[i].Label, sNpcFilter) &&
                !MatchesFilter(sNpcOptions[i].ItemLabel, sNpcFilter) &&
                !MatchesFilter(sNpcOptions[i].NativeId, sNpcFilter)) {
                continue;
            }
            const bool collected = OotmmNpcChecks_IsCollected(sNpcOptions[i].NativeId.c_str()) != 0;
            const std::string label = collected ? sNpcOptions[i].Label + " (collected)" : sNpcOptions[i].Label;
            if (ImGui::Selectable(label.c_str(), i == sNpcSelection)) {
                sNpcSelection = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    const NpcOption& selected = sNpcOptions[sNpcSelection];
    ImGui::BeginDisabled(gPlayState == nullptr || selected.Entrance < 0);
    if (ImGui::Button("Go")) {
        const int32_t entrance = OotmmSession_ApplyResolvedOotEntrance(selected.Entrance);
        if (entrance >= 0) {
            GameInteractor::RawAction::TeleportPlayer(entrance);
        }
    }
    ImGui::EndDisabled();
    const bool collected = OotmmNpcChecks_IsCollected(selected.NativeId.c_str()) != 0;
    if (collected) {
        ImGui::SameLine();
        ImGui::TextDisabled("Collected");
    }
    ImGui::TextDisabled("Contains: %s", selected.ItemLabel.c_str());
    if (selected.RouteHint != nullptr) {
        ImGui::TextDisabled("%s", selected.RouteHint);
    } else if (selected.Entrance < 0) {
        ImGui::TextDisabled("This check has no routable scene in the seed.");
    }
}

} // namespace

void OotmmDebugChecks_DrawTab() {
    ImGui::TextWrapped(
        "Places test chests for previewing check assets. They open like real ones and grant nothing.");
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
    if (ImGui::BeginCombo("Appearance", kAppearances[sAppearance].Label)) {
        for (int i = 0; i < static_cast<int>(std::size(kAppearances)); i++) {
            if (ImGui::Selectable(kAppearances[i].Label, i == sAppearance)) {
                sAppearance = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(gPlayState == nullptr);
    if (ImGui::Button("Place chest")) {
        PlaceChest();
    }
    ImGui::EndDisabled();
    if (!sDebugChests.empty()) {
        ImGui::TextDisabled("%d debug chest(s) in this scene.", static_cast<int>(sDebugChests.size()));
    }
    DrawDropSection();
    DrawNpcSection();
}

#if !defined(NDEBUG) || defined(OOTMM_RUNTIME_TEST)
extern "C" int OotmmDebugChecks_ReadRuntimePad(uint16_t* buttons, int8_t* x, int8_t* y) {
    const uint64_t value = sRuntimePad.load(std::memory_order_relaxed);
    if ((value & (1ull << 32)) == 0) {
        return 0;
    }
    *buttons = static_cast<uint16_t>(value);
    *x = static_cast<int8_t>(value >> 16);
    *y = static_cast<int8_t>(value >> 24);
    return 1;
}

static void ApplySetupString(const char* variable, const std::string& setup);

static int sDialogResult = -1;

static void StartTestDialog(const std::string& name) {
    const char* directory = std::getenv("OOTMM_RUNTIME_TEST_DIR");
    if (directory == nullptr || gPlayState == nullptr) {
        return;
    }
    std::ifstream file(std::filesystem::u8path(directory) / ("dialog-" + name + ".json"), std::ios::binary);
    const auto spec = nlohmann::json::parse(file, nullptr, false);
    if (!spec.is_object()) {
        return;
    }
    sDialogResult = -1;
    OotmmDialog::Start(gPlayState, spec.value("body", std::string()),
                       spec.value("options", std::vector<std::string>()), [](int choice) { sDialogResult = choice; });
}

static void ApplyRuntimeSetup(const char* variable) {
    const char* probe = std::getenv(variable);
    const char* directory = std::getenv("OOTMM_RUNTIME_TEST_DIR");
    if (probe == nullptr || directory == nullptr) {
        return;
    }
    const auto marker = std::filesystem::u8path(directory) / (std::string(variable) + ".applied");
    if (std::filesystem::exists(marker)) {
        return;
    }
    std::ofstream(marker) << probe;
    ApplySetupString(variable, probe);
}

static void ApplyLiveSetup() {
    const char* directory = std::getenv("OOTMM_RUNTIME_TEST_DIR");
    if (directory == nullptr) {
        return;
    }
    const auto path = std::filesystem::u8path(directory) / "live-setup.txt";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return;
    }
    std::string setup;
    std::getline(std::ifstream(path), setup);
    std::filesystem::remove(path, ec);
    ApplySetupString("live-setup", setup);
}

static void ApplySetupString(const char* variable, const std::string& setup) {
    nlohmann::json applied = nlohmann::json::array();
    size_t start = 0;
    while (start < setup.size()) {
        const size_t end = std::min(setup.find(',', start), setup.size());
        const std::string token = setup.substr(start, end - start);
        start = end + 1;
        const size_t equals = token.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const std::string key = token.substr(0, equals);
        const std::string value = token.substr(equals + 1);
        const size_t colon = value.find(':');
        const long first = std::strtol(value.c_str(), nullptr, 0);
        const long second = colon == std::string::npos ? 0 : std::strtol(value.c_str() + colon + 1, nullptr, 0);
        if (key == "remote") {
            Ship::GameIpcItemPresentation presentation;
            presentation.ItemId = value;
            presentation.ItemName = value;
            presentation.ItemGame = value.starts_with("MM_") ? "mm" : "oot";
            for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
                if (placement.ItemId == value) {
                    presentation.ItemName = placement.ItemName;
                    presentation.ItemGame = placement.ItemGame;
                    break;
                }
            }
            presentation.DestinationGame = presentation.ItemGame == "mm" || presentation.ItemGame == "oot"
                                               ? presentation.ItemGame
                                               : "oot";
            presentation.SourcePlayer = 2;
            presentation.RecipientPlayer = 1;
            presentation.ViewerPlayer = 1;
            presentation.Background = true;
            OotmmItemPresentation_Queue(presentation);
        } else if (key == "drop") {
            const Ship::OotmmPlacement* match = nullptr;
            for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
                if (placement.ItemId == value) {
                    match = &placement;
                    break;
                }
            }
            Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
            if (match == nullptr || player == nullptr) {
                continue;
            }
            const float x = player->actor.world.pos.x + Math_SinS(player->actor.shape.rot.y) * 60.0f;
            const float z = player->actor.world.pos.z + Math_CosS(player->actor.shape.rot.y) * 60.0f;
            if (!OotmmDrops_Spawn(gPlayState, x, player->actor.world.pos.y + 20.0f, z, match->ItemId, match->ItemName,
                                  match->ItemGame, "")) {
                continue;
            }
        } else if (key == "bsword" && first >= ITEM_SWORD_KOKIRI && first <= ITEM_SWORD_BGS) {
            gSaveContext.inventory.equipment |= static_cast<u16>(1 << (first - ITEM_SWORD_KOKIRI));
            gSaveContext.equips.buttonItems[0] = static_cast<u8>(first);
            gSaveContext.equips.equipment = (gSaveContext.equips.equipment & ~0xF) |
                                            static_cast<u16>(first - ITEM_SWORD_KOKIRI + 1);
            Flags_UnsetInfTable(INFTABLE_SWORDLESS);
            if (gPlayState != nullptr) {
                Interface_LoadItemIcon1(gPlayState, 0);
            }
        } else if (key == "c" && first >= 0 && first < 3 && second >= 0 &&
                   (second < static_cast<long>(ARRAY_COUNT(gSaveContext.inventory.items)) ||
                    OotmmCustomItems_IsCustomSlot(static_cast<uint8_t>(second)))) {
            gSaveContext.equips.cButtonSlots[first] = static_cast<u8>(second);
            gSaveContext.equips.buttonItems[first + 1] =
                OotmmCustomItems_IsCustomSlot(static_cast<uint8_t>(second))
                    ? OotmmCustomItems_ItemInSlot(static_cast<uint8_t>(second))
                    : gSaveContext.inventory.items[second];
            if (gPlayState != nullptr) {
                Interface_LoadItemIcon1(gPlayState, static_cast<u16>(first + 1));
            }
        } else if (key == "reload" && gPlayState != nullptr) {
            gPlayState->nextEntranceIndex = gSaveContext.entranceIndex;
            gPlayState->transitionTrigger = TRANS_TRIGGER_START;
            gPlayState->transitionType = TRANS_TYPE_FADE_BLACK;
        } else if (key == "grotto") {
            gSaveContext.respawn[RESPAWN_MODE_RETURN].data = static_cast<s8>(first);
        } else if (key == "time") {
            gSaveContext.dayTime = static_cast<u16>(first);
        } else if (key == "rupees") {
            gSaveContext.rupees = static_cast<s16>(first);
        } else if (key == "health") {
            gSaveContext.health = static_cast<s16>(first);
        } else if (key == "magic") {
            gSaveContext.magic = static_cast<s8>(first);
        } else if (key == "quest" && first >= 0 && first < 32) {
            gSaveContext.inventory.questItems |= 1u << first;
        } else if (key == "equip") {
            gSaveContext.inventory.equipment |= static_cast<u16>(first);
        } else if (key == "switch" && first >= 0 && first < static_cast<long>(ARRAY_COUNT(gSaveContext.sceneFlags)) &&
                   second >= 0 && second < 64) {
            if (second < 32) {
                gSaveContext.sceneFlags[first].swch |= 1u << second;
            }
            if (gPlayState != nullptr && gPlayState->sceneNum == first) {
                Flags_SetSwitch(gPlayState, static_cast<s32>(second));
            }
        } else if (key == "unchest" && first >= 0 &&
                   first < static_cast<long>(ARRAY_COUNT(gSaveContext.sceneFlags)) && second >= 0 && second < 32) {
            gSaveContext.sceneFlags[first].chest &= ~(1u << second);
            if (gPlayState != nullptr && gPlayState->sceneNum == first) {
                gPlayState->actorCtx.flags.chest &= ~(1u << second);
            }
        } else if (key == "clear" && first >= 0 && first < static_cast<long>(ARRAY_COUNT(gSaveContext.sceneFlags)) &&
                   second >= 0 && second < 32) {
            gSaveContext.sceneFlags[first].clear |= 1u << second;
            if (gPlayState != nullptr && gPlayState->sceneNum == first) {
                Flags_SetClear(gPlayState, static_cast<s32>(second));
                Flags_SetTempClear(gPlayState, static_cast<s32>(second));
            }
        } else if (key == "gear" && first >= EQUIP_TYPE_SWORD && first <= EQUIP_TYPE_BOOTS) {
            Inventory_ChangeEquipment(static_cast<s16>(first), static_cast<u16>(second));
        } else if (key == "wear" && first >= EQUIP_TYPE_SWORD && first <= EQUIP_TYPE_BOOTS) {
            Inventory_ChangeEquipment(static_cast<s16>(first), static_cast<u16>(second));
            if (gPlayState != nullptr) {
                Player_SetEquipmentData(gPlayState, GET_PLAYER(gPlayState));
            }
        } else if (key == "item" && first >= 0 && first < static_cast<long>(ARRAY_COUNT(gSaveContext.inventory.items))) {
            gSaveContext.inventory.items[first] = static_cast<u8>(second);
        } else if (key == "ammo" && first >= 0 && first < static_cast<long>(ARRAY_COUNT(gSaveContext.inventory.ammo))) {
            gSaveContext.inventory.ammo[first] = static_cast<s8>(second);
        } else if (key == "upgrade") {
            Inventory_ChangeUpgrade(static_cast<s16>(first), static_cast<s16>(second));
        } else if (key == "invitem") {
            const auto colon = value.find(':');
            if (colon != std::string::npos) {
                OotmmIpc_SetDebugItemValue(value.substr(0, colon),
                                           static_cast<uint32_t>(std::strtoul(value.c_str() + colon + 1, nullptr, 0)));
            }
        } else if (key == "poly") {
            sDebugPoly = static_cast<s32>(first);
        } else if (key == "collect") {
            OotmmNpcChecks_Collect(value.c_str());
        } else if (key == "goto" && gPlayState != nullptr) {
            gPlayState->nextEntranceIndex = static_cast<u16>(first);
            gPlayState->transitionTrigger = TRANS_TRIGGER_START;
            gPlayState->transitionType = TRANS_TYPE_FADE_BLACK;
        } else if (key == "bombnear" && gPlayState != nullptr) {
            Player* holder = GET_PLAYER(gPlayState);
            Actor* best = nullptr;
            f32 bestDist = 0.0f;
            for (auto& list : gPlayState->actorCtx.actorLists) {
                for (Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                    if (actor->id == first) {
                        f32 dist = Actor_WorldDistXZToActor(&holder->actor, actor);
                        if (best == nullptr || dist < bestDist) {
                            best = actor;
                            bestDist = dist;
                        }
                    }
                }
            }
            if (best != nullptr) {
                auto* bomb = reinterpret_cast<EnBom*>(Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_BOM, best->world.pos.x,
                                                                  best->world.pos.y + 10.0f, best->world.pos.z, 0, 0, 0, 0));
                if (bomb != nullptr) {
                    bomb->timer = 1;
                    bomb->explosionCollider.elements[0].dim.worldSphere.center.x = static_cast<s16>(best->world.pos.x);
                    bomb->explosionCollider.elements[0].dim.worldSphere.center.y = static_cast<s16>(best->world.pos.y + 10.0f);
                    bomb->explosionCollider.elements[0].dim.worldSphere.center.z = static_cast<s16>(best->world.pos.z);
                }
            }
        } else if (key == "bombat" && gPlayState != nullptr) {
            for (auto& list : gPlayState->actorCtx.actorLists) {
                for (Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                    if (actor->id == first && (second == 0 || (actor->params & 0xFFFF) == second)) {
                        auto* bomb = reinterpret_cast<EnBom*>(Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_BOM, actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, 0, 0, 0, 0));
                        if (bomb != nullptr) {
                            bomb->timer = 1;
                            bomb->explosionCollider.elements[0].dim.worldSphere.center.x = static_cast<s16>(actor->world.pos.x);
                            bomb->explosionCollider.elements[0].dim.worldSphere.center.y = static_cast<s16>(actor->world.pos.y);
                            bomb->explosionCollider.elements[0].dim.worldSphere.center.z = static_cast<s16>(actor->world.pos.z);
                        }
                        break;
                    }
                }
            }
        } else if (key == "bombh" && gPlayState != nullptr) {
            for (auto& list : gPlayState->actorCtx.actorLists) {
                for (Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                    if (actor->id == first) {
                        auto* bomb = reinterpret_cast<EnBom*>(Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_BOM, actor->world.pos.x, actor->world.pos.y + second, actor->world.pos.z, 0, 0, 0, 0));
                        if (bomb != nullptr) {
                            bomb->timer = 1;
                            bomb->explosionCollider.elements[0].dim.worldSphere.center.x = static_cast<s16>(actor->world.pos.x);
                            bomb->explosionCollider.elements[0].dim.worldSphere.center.y = static_cast<s16>(actor->world.pos.y + second);
                            bomb->explosionCollider.elements[0].dim.worldSphere.center.z = static_cast<s16>(actor->world.pos.z);
                        }
                        break;
                    }
                }
            }
        } else if (key == "invuln" && gPlayState != nullptr) {
            GET_PLAYER(gPlayState)->invincibilityTimer = static_cast<s8>(first);
        } else if (key == "aim" && gPlayState != nullptr) {
            Player* player = GET_PLAYER(gPlayState);
            player->actor.focus.rot.x = static_cast<s16>(first);
            player->actor.focus.rot.y = static_cast<s16>(second);
        } else if (key == "acthp" && gPlayState != nullptr) {
            for (auto& list : gPlayState->actorCtx.actorLists) {
                for (Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                    if (actor->id == first) {
                        actor->colChkInfo.health = static_cast<u8>(second);
                    }
                }
            }
        } else if (key == "hba") {
            gSaveContext.eventInf[0] |= 0x100;
            gSaveContext.minigameScore = static_cast<s32>(first);
        } else if (key == "echk") {
            Flags_SetEventChkInf(static_cast<s32>(first));
        } else if (key == "inftable") {
            Flags_SetInfTable(static_cast<s32>(first));
        } else if (key == "memorycheat") {
            CVarSetInteger(CVAR_ENHANCEMENT("CustomizeOcarinaGame"), 1);
            CVarSetInteger(CVAR_ENHANCEMENT("InstantOcarinaGameWin"), 1);
        } else if (key == "fishnear" && gPlayState != nullptr) {
            const Player* player = GET_PLAYER(gPlayState);
            const float yaw = player->actor.shape.rot.y * static_cast<float>(M_PI) / 0x8000;
            int index = 0;
            for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
                if (actor->id == ACTOR_FISHING && actor->params >= 100 && actor->params < 200 &&
                    (first == 0 || actor->params == first)) {
                    const float reach = (second != 0 ? second : 150) + 12.0f * index++;
                    actor->world.pos.x = player->actor.world.pos.x + sinf(yaw) * reach;
                    actor->world.pos.z = player->actor.world.pos.z + cosf(yaw) * reach;
                    actor->world.pos.y = -30.0f;
                    actor->home.pos = actor->prevPos = actor->world.pos;
                }
            }
        } else if (key == "labdive" && gPlayState != nullptr) {
            for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
                if (actor->id == ACTOR_EN_MK) {
                    reinterpret_cast<EnMk*>(actor)->swimFlag = static_cast<s16>(first);
                }
            }
        } else if (key == "dnthit" && gPlayState != nullptr) {
            for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_PROP].head; actor != nullptr; actor = actor->next) {
                if (actor->id == ACTOR_EN_DNT_NOMAL) {
                    auto* dnt = reinterpret_cast<EnDntNomal*>(actor);
                    dnt->targetQuad.base.acFlags |= AC_HIT;
                    dnt->targetQuad.info.bumper.hitPos.x = 1340;
                    dnt->targetQuad.info.bumper.hitPos.y = 50;
                    dnt->targetQuad.info.bumper.hitPos.z = -30;
                }
            }
        } else if (key == "treasure" && gPlayState != nullptr) {
            Flags_SetTreasure(gPlayState, static_cast<s32>(first));
        } else if (key == "fishcheat") {
            CVarSetInteger(CVAR_ENHANCEMENT("CustomizeFishing"), 1);
            CVarSetInteger(CVAR_ENHANCEMENT("InstantFishing"), 1);
            CVarSetInteger(CVAR_ENHANCEMENT("GuaranteeFishingBite"), 1);
            CVarSetInteger(CVAR_ENHANCEMENT("FishNeverEscape"), 1);
        } else if (key == "breg") {
            BREG(first) = static_cast<s16>(second);
        } else if (key == "dog") {
            gSaveContext.dogParams = static_cast<u16>(first);
            gSaveContext.dogIsLost = true;
        } else if (key == "minigame") {
            gSaveContext.minigameScore = static_cast<s32>(first);
            BREG(2) = 1;
            if (gPlayState != nullptr) {
                for (auto& list : gPlayState->actorCtx.actorLists) {
                    for (Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                        if (actor->id == ACTOR_EN_DIVING_GAME) {
                            reinterpret_cast<EnDivingGame*>(actor)->grabbedRupeesCounter = static_cast<s16>(first);
                        }
                        if (actor->id == ACTOR_EN_SYATEKI_ITM) {
                            auto* gallery = reinterpret_cast<EnSyatekiItm*>(actor);
                            gallery->hitCount = static_cast<s16>(first);
                            gallery->signal = ENSYATEKI_END;
                        }
                    }
                }
            }
        } else if (key == "dialog") {
            StartTestDialog(value);
        } else if (key == "event" && first >= 0 && first < 0x100) {
            Flags_SetEventChkInf(static_cast<s32>(first));
        } else if (key == "getinf" && first >= 0 &&
                   first < static_cast<long>(ARRAY_COUNT(gSaveContext.itemGetInf) * 16)) {
            Flags_SetItemGetInf(static_cast<s32>(first));
        } else if (key == "tokens") {
            gSaveContext.inventory.gsTokens = static_cast<s16>(first);
        } else if (key == "ruto") {
            OotmmItemApply_LedgerSet("native:rutoLetterUsed", static_cast<uint32_t>(first));
        } else {
            continue;
        }
        applied.push_back(token);
    }
    Ship::OotmmRuntimeTest::Get().Observe(
        { { "event", "runtimeSetup" }, { "variable", variable }, { "applied", applied } });
}
#endif

void OotmmDebugChecks_Init() {
#if !defined(NDEBUG) || defined(OOTMM_RUNTIME_TEST)
    if (Ship::OotmmRuntimeTest::Get().Enabled()) {
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([]() {
            static int settledFrames = 0;
            if (settledFrames <= 30 && OotmmSession_IsActive() && ++settledFrames > 30) {
                ApplyRuntimeSetup("OOTMM_RUNTIME_OOT_SETUP");
            }
            if (gPlayState != nullptr && gPlayState->gameplayFrames % 10 == 0) {
                ApplyLiveSetup();
            }
        });
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>([]() {
            if (gGameState == nullptr || !OotmmSession_IsActive()) {
                sRuntimePad.store(0, std::memory_order_relaxed);
                return;
            }
            static OSContPad previous{};
            const auto pad = Ship::OotmmRuntimeTest::Get().Advance();
            static int pendingRoom = -1;
            if (const auto room = Ship::OotmmRuntimeTest::Get().TakeRoom();
                room && gPlayState != nullptr && *room < gPlayState->numRooms &&
                *room != gPlayState->roomCtx.curRoom.num && func_8009728C(gPlayState, &gPlayState->roomCtx, *room)) {
                pendingRoom = *room;
            }
            if (pendingRoom >= 0 && gPlayState != nullptr && gPlayState->roomCtx.status == 0 &&
                gPlayState->roomCtx.curRoom.num == pendingRoom) {
                func_80097534(gPlayState, &gPlayState->roomCtx);
                pendingRoom = -1;
            }
            if (const auto warp = Ship::OotmmRuntimeTest::Get().TakeWarp(); warp && gPlayState != nullptr) {
                Player* player = GET_PLAYER(gPlayState);
                player->actor.world.pos = { warp->X, warp->Y, warp->Z };
                player->actor.prevPos = player->actor.home.pos = player->actor.world.pos;
                player->actor.world.rot.y = player->actor.shape.rot.y = player->yaw = warp->Yaw;
                player->actor.velocity = { 0.0f, 0.0f, 0.0f };
                player->actor.speedXZ = player->linearVelocity = 0.0f;
                if (const char* launch = std::getenv("OOTMM_RUNTIME_WARP_SPEED"); launch != nullptr) {
                    player->actor.speedXZ = player->linearVelocity = std::strtof(launch, nullptr);
                }
            }
            sRuntimePad.store((1ull << 32) | pad.Buttons |
                (static_cast<uint64_t>(static_cast<uint8_t>(pad.X)) << 16) |
                (static_cast<uint64_t>(static_cast<uint8_t>(pad.Y)) << 24), std::memory_order_relaxed);
            auto& input = gGameState->input[0];
            input = {};
            input.prev = previous;
            input.cur.button = pad.Buttons;
            input.cur.stick_x = pad.X;
            input.cur.stick_y = pad.Y;
            input.cur.right_stick_y = pad.RightY;
            input.press.button = input.cur.button & ~previous.button;
            input.rel.button = previous.button & ~input.cur.button;
            input.press.stick_x = input.cur.stick_x - previous.stick_x;
            input.press.stick_y = input.cur.stick_y - previous.stick_y;
            PadUtils_UpdateRelXY(&input);
            previous = input.cur;
            if (gGameState->destroy == FileChoose_Destroy) {
                const auto* file = reinterpret_cast<const FileChooseContext*>(gGameState);
                nlohmann::json observation = {
                    { "game", "oot" }, { "mode", "fileSelect" }, { "buttons", input.cur.button },
                    { "menuMode", file->menuMode }, { "configMode", file->configMode },
                    { "selectMode", file->selectMode }, { "buttonIndex", file->buttonIndex },
                    { "confirmButtonIndex", file->confirmButtonIndex }, { "fileIndex", file->selectedFileIndex }
                };
                if (file->configMode == 33) {
                    observation["nameLength"] = file->newFileNameCharCount;
                    observation["keyboardX"] = file->kbdX;
                    observation["keyboardY"] = file->kbdY;
                }
                Ship::OotmmRuntimeTest::Get().Observe(std::move(observation));
            } else if (gGameState->destroy != Play_Destroy || gSaveContext.gameMode != GAMEMODE_NORMAL) {
                Ship::OotmmRuntimeTest::Get().Observe({
                    { "game", "oot" }, { "mode", "boot" }, { "buttons", input.cur.button },
                    { "gameMode", gSaveContext.gameMode }
                });
            }
        });
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([]() {
            if (gGameState == nullptr || gGameState->destroy != Play_Destroy || gPlayState == nullptr ||
                !OotmmSession_IsActive() || gSaveContext.gameMode != GAMEMODE_NORMAL) {
                return;
            }
            const Player* player = GET_PLAYER(gPlayState);
            if (player == nullptr) {
                return;
            }
            static bool magicSaveAttempted = false;
            const char* magicSaveProbe = std::getenv("OOTMM_RUNTIME_SAVE_DURING_MAGIC");
            const std::string magicSavePhase = magicSaveProbe != nullptr ? magicSaveProbe : "";
            const bool magicSaveReady =
                (magicSavePhase == "initial" && gSaveContext.isMagicAcquired &&
                 gSaveContext.magicLevel == 0 && gSaveContext.magicState == MAGIC_STATE_IDLE) ||
                (magicSavePhase == "add" && gSaveContext.magicState == MAGIC_STATE_ADD) ||
                (magicSavePhase == "capacity" && gSaveContext.magicState == MAGIC_STATE_STEP_CAPACITY) ||
                (magicSavePhase == "fill" && gSaveContext.magicState == MAGIC_STATE_FILL);
            if (!magicSaveAttempted && magicSaveReady) {
                magicSaveAttempted = true;
                const auto beforeMagic = gSaveContext.magic;
                const auto magicState = gSaveContext.magicState;
                const auto fillTarget = gSaveContext.magicFillTarget;
                const auto addTarget = gSaveContext.magicTarget;
                const auto capacity = gSaveContext.magicCapacity;
                const auto magicLevel = gSaveContext.magicLevel;
                Play_PerformSave(gPlayState);
                SaveManager::Instance->ThreadPoolWait();
                Ship::OotmmRuntimeTest::Get().Observe({ { "event", "pendingMagicSave" },
                    { "magicBefore", beforeMagic }, { "magicState", magicState },
                    { "phase", magicSavePhase }, { "fillTarget", fillTarget },
                    { "magicLevel", magicLevel },
                    { "addTarget", addTarget }, { "capacity", capacity },
                    { "saved", SaveManager::Instance->LastBaseSaveSucceeded(gSaveContext.fileNum) } });
            }
            static bool respawnPlaced = false;
            const char* respawnProbe = std::getenv("OOTMM_RUNTIME_RESPAWN");
            int respawnEntrance = gSaveContext.entranceIndex;
            int respawnRoom = 0;
            Vec3f respawnPos = {};
            int probeEntrance = 0;
            int probeRoom = 0;
            Vec3f probePos = {};
            bool respawnParsed = false;
            if (respawnProbe != nullptr && std::sscanf(respawnProbe, "%i,%d,%f,%f,%f", &probeEntrance, &probeRoom,
                                                       &probePos.x, &probePos.y, &probePos.z) == 5) {
                respawnEntrance = probeEntrance;
                respawnRoom = probeRoom;
                respawnPos = probePos;
                respawnParsed = true;
            } else if (respawnProbe != nullptr && std::sscanf(respawnProbe, "%d,%f,%f,%f", &probeRoom, &probePos.x,
                                                              &probePos.y, &probePos.z) == 4) {
                respawnRoom = probeRoom;
                respawnPos = probePos;
                respawnParsed = true;
            }
            if (!respawnPlaced && respawnParsed && gPlayState->transitionTrigger == TRANS_TRIGGER_OFF) {
                respawnPlaced = true;
                Play_SetRespawnData(gPlayState, RESPAWN_MODE_DOWN, respawnEntrance, respawnRoom, 0xDFF, &respawnPos,
                                    player->actor.shape.rot.y);
                Play_TriggerVoidOut(gPlayState);
                Ship::OotmmRuntimeTest::Get().Observe({ { "event", "respawnPlaced" }, { "entrance", respawnEntrance }, { "room", respawnRoom },
                                                        { "x", respawnPos.x }, { "y", respawnPos.y },
                                                        { "z", respawnPos.z } });
            }
            if (const char* floor = std::getenv("OOTMM_RUNTIME_MIN_HEALTH"); floor != nullptr) {
                gSaveContext.health = std::max<s16>(gSaveContext.health, static_cast<s16>(std::atoi(floor)));
            }
            static u8 sfxReadIndex = sSoundRequestWriteIndex;
            static u32 sfxSerial = 0;
            static std::vector<std::pair<u32, u16>> recentSfx;
            while (sfxReadIndex != sSoundRequestWriteIndex) {
                recentSfx.emplace_back(++sfxSerial, sSoundRequests[sfxReadIndex].sfxId);
                if (recentSfx.size() > 64) {
                    recentSfx.erase(recentSfx.begin());
                }
                sfxReadIndex++;
            }
            int clampId = 0;
            int clampHp = 0;
            if (const char* clamp = std::getenv("OOTMM_RUNTIME_ACTOR_HP");
                clamp != nullptr && std::sscanf(clamp, "%i,%d", &clampId, &clampHp) == 2) {
                for (auto& list : gPlayState->actorCtx.actorLists) {
                    for (Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                        if (actor->id != clampId) {
                            continue;
                        }
                        int limit = clampHp;
                        if (actor->id == ACTOR_BOSS_GANON2 && reinterpret_cast<BossGanon2*>(actor)->unk_334 == 0) {
                            limit = std::max(limit, 0x15);
                        }
                        if (static_cast<s8>(actor->colChkInfo.health) > limit) {
                            actor->colChkInfo.health = static_cast<u8>(limit);
                        }
                    }
                }
            }
            if (!Ship::OotmmRuntimeTest::Get().ShouldObserve()) {
                return;
            }
            nlohmann::json checks = nlohmann::json::array();
            for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
                if (placement.CheckGame == Ship::OotmmGame::Oot &&
                    OotmmIpc_IsCheckCompleted(placement.CheckId)) {
                    checks.push_back(placement.CheckId);
                }
            }
            nlohmann::json actors = nlohmann::json::array();
            nlohmann::json sunHitboxes = nlohmann::json::array();
            nlohmann::json ganonTail = nullptr;
            nlohmann::json kingDodongo = nullptr;
            nlohmann::json tokenDbg = nullptr;
            nlohmann::json polyDbg = nullptr;
            if (sDebugPoly >= 0 && gPlayState->colCtx.colHeader != nullptr &&
                sDebugPoly < gPlayState->colCtx.colHeader->numPolygons) {
                const auto* header = gPlayState->colCtx.colHeader;
                const auto& poly = header->polyList[sDebugPoly];
                const auto& a = header->vtxList[COLPOLY_VTX_INDEX(poly.flags_vIA)];
                const auto& b = header->vtxList[COLPOLY_VTX_INDEX(poly.flags_vIB)];
                const auto& c = header->vtxList[poly.vIC];
                polyDbg = { { "type", poly.type }, { "flagsA", poly.flags_vIA >> 13 }, { "flagsB", poly.flags_vIB >> 13 },
                            { "count", header->numPolygons },
                            { "a", { a.x, a.y, a.z } }, { "b", { b.x, b.y, b.z } }, { "c", { c.x, c.y, c.z } } };
            }
            for (const auto& list : gPlayState->actorCtx.actorLists) {
                for (const Actor* actor = list.head; actor != nullptr; actor = actor->next) {
                    actors.push_back({ { "id", actor->id }, { "params", actor->params },
                                       { "x", actor->world.pos.x }, { "y", actor->world.pos.y },
                                       { "z", actor->world.pos.z }, { "rx", actor->shape.rot.x }, { "ry", actor->shape.rot.y },
                                       { "draw", actor->draw != nullptr }, { "hp", actor->colChkInfo.health }, { "wrz", actor->world.rot.z },
                                       { "xflag", OotmmXflags_NativeId(const_cast<Actor*>(actor)) },
                                       { "shuffled", OotmmXflags_IsShuffled(const_cast<Actor*>(actor)) != 0 } });
                    if (actor->id == ACTOR_EN_SI) {
                        const auto* si = reinterpret_cast<const EnSi*>(actor);
                        tokenDbg = { { "oc1", si->collider.base.ocFlags1 }, { "oc2", si->collider.base.ocFlags2 },
                                     { "flags", actor->flags }, { "freeze", actor->freezeTimer },
                                     { "cy", si->collider.dim.pos.y }, { "cr", si->collider.dim.radius },
                                     { "ch", si->collider.dim.height }, { "cs", si->collider.dim.yShift } };
                    }
                    if (gOotmmBomMmId >= 0 && actor->id == gOotmmBomMmId) {
                        const auto* bomb = reinterpret_cast<const OotmmBomMm*>(actor);
                        actors.back()["timer"] = bomb->timer;
                        actors.back()["keg"] = bomb->isPowderKeg;
                        actors.back()["radius"] = bomb->collider2.elements[0].dim.worldSphere.radius;
                        actors.back()["atFlags"] = bomb->collider2.base.atFlags;
                        actors.back()["speed"] = actor->speedXZ;
                        actors.back()["scale"] = actor->scale.x;
                    }
                    if (actor->id == ACTOR_EN_DODONGO) {
                        actors.back()["eatingId"] = actor->child != nullptr ? actor->child->id : (actor->parent != nullptr ? actor->parent->id : -1);
                        const auto* dodongo = reinterpret_cast<const EnDodongo*>(actor);
                        actors.back()["mouth"] = { dodongo->mouthPos.x, dodongo->mouthPos.y, dodongo->mouthPos.z };
                        actors.back()["state"] = dodongo->actionState;
                    }
                    if (actor->id == ACTOR_BOSS_DODONGO && actor->init == nullptr) {
                        const auto* kd = reinterpret_cast<const BossDodongo*>(actor);
                        kingDodongo = { { "health", kd->health }, { "inhaling", kd->unk_1E2 }, { "inhaleTimer", kd->unk_1AC },
                                        { "timer", kd->unk_1DA }, { "dying", kd->csState },
                                        { "mouth", { { "x", kd->mouthPos.x }, { "y", kd->mouthPos.y }, { "z", kd->mouthPos.z } } } };
                    }
                    if (actor->id == ACTOR_BOSS_GANON2 && actor->init == nullptr) {
                        const auto* ganon = reinterpret_cast<const BossGanon2*>(actor);
                        const auto& tail = ganon->unk_464[15].dim.worldSphere;
                        ganonTail = { { "x", tail.center.x }, { "y", tail.center.y }, { "z", tail.center.z },
                                      { "radius", tail.radius }, { "lookOn", ganon->unk_313 },
                                      { "swordBack", ganon->unk_334 },
                                      { "head", { { "x", ganon->unk_1B8.x }, { "z", ganon->unk_1B8.z } } } };
                        if (const auto* effects = static_cast<const BossGanon2Effect*>(gPlayState->specialEffects);
                            effects != nullptr) {
                            for (int i = 0; i < 100; i++) {
                                if (effects[i].type == 1 && effects[i].unk_2E != 0) {
                                    ganonTail["sword"] = { { "x", effects[i].position.x },
                                                           { "y", effects[i].position.y },
                                                           { "z", effects[i].position.z } };
                                }
                            }
                        }
                    }
                    if (actor->id == ACTOR_SHOT_SUN && (actor->params & 0xFF) == 1) {
                        const auto* sun = reinterpret_cast<const ShotSun*>(actor);
                        sunHitboxes.push_back({
                            { "x", sun->hitboxPos.x }, { "y", sun->hitboxPos.y }, { "z", sun->hitboxPos.z },
                            { "radius", sun->collider.dim.radius }, { "height", sun->collider.dim.height },
                            { "acFlags", sun->collider.base.acFlags }
                        });
                    }
                }
            }
            nlohmann::json observation = {
                { "game", "oot" }, { "mode", "gameplay" },
                { "age", gSaveContext.linkAge }, { "entrance", gSaveContext.entranceIndex },
                { "returnData", gSaveContext.respawn[RESPAWN_MODE_RETURN].data },
                { "eventChkInf", gSaveContext.eventChkInf },
                { "fw", { { "set", gSaveContext.fw.set }, { "entrance", gSaveContext.fw.entranceIndex },
                          { "x", gSaveContext.fw.pos.x }, { "y", gSaveContext.fw.pos.y }, { "z", gSaveContext.fw.pos.z } } },
                { "weather", gWeatherMode },
                { "setupIndex", gSaveContext.sceneSetupIndex },
                { "eventWaterWarp", Flags_GetEventChkInf(EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP) },
                { "eventFireWarp", Flags_GetEventChkInf(EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP) },
                { "sandstorm", gPlayState->envCtx.sandstormState },
                { "scene", gPlayState->sceneNum }, { "room", gPlayState->roomCtx.curRoom.num },
                { "rooms", gPlayState->numRooms },
                { "meleeAnim", player->meleeWeaponAnimation }, { "switches", gPlayState->actorCtx.flags.swch }, { "tempSwitches", gPlayState->actorCtx.flags.tempSwch },
                { "health", gSaveContext.health }, { "rupees", gSaveContext.rupees },
                { "healthCapacity", gSaveContext.healthCapacity }, { "ammo", gSaveContext.inventory.ammo },
                { "owned", OotmmIpc_GetInventory().GetValues() },
                { "items", gSaveContext.inventory.items }, { "questItems", gSaveContext.inventory.questItems },
                { "heartPieces", (gSaveContext.inventory.questItems >> 28) & 0xF },
                { "dungeonKeys", gSaveContext.inventory.dungeonKeys },
                { "gsTokens", gSaveContext.inventory.gsTokens },
                { "dungeonItems", gSaveContext.inventory.dungeonItems },
                { "waterBoxes", [] {
                     nlohmann::json boxes = nlohmann::json::array();
                     const CollisionHeader* header = gPlayState->colCtx.colHeader;
                     for (u16 i = 0; header != nullptr && i < header->numWaterBoxes && i < 12; i++) {
                         const WaterBox& box = header->waterBoxes[i];
                         boxes.push_back({ box.xMin, box.ySurface, box.zMin, box.xLength, box.zLength });
                     }
                     return boxes;
                 }() },
                { "trapState", {
                    { "burning", GET_PLAYER(gPlayState)->bodyIsBurning },
                    { "shock", GET_PLAYER(gPlayState)->bodyShockTimer },
                    { "invincibility", GET_PLAYER(gPlayState)->invincibilityTimer },
                    { "applied", OotmmItemApply_TrapsApplied() }
                } },
                { "nativeMagic", {
                    { "value", gSaveContext.magic }, { "level", gSaveContext.magicLevel },
                    { "acquired", gSaveContext.isMagicAcquired },
                    { "double", gSaveContext.isDoubleMagicAcquired },
                    { "capacity", gSaveContext.magicCapacity }, { "state", gSaveContext.magicState },
                    { "fillTarget", gSaveContext.magicFillTarget }, { "addTarget", gSaveContext.magicTarget }
                } },
                { "saveSucceeded", SaveManager::Instance->LastBaseSaveSucceeded(gSaveContext.fileNum) },
                { "dialog",
                  [] {
                      const auto dialog = OotmmDialog::Debug();
                      return nlohmann::json{ { "active", dialog.Active },       { "top", dialog.Top },
                                             { "total", dialog.Total },         { "choice", dialog.Choice },
                                             { "cursorRow", dialog.CursorRow }, { "scrollable", dialog.Scrollable },
                                             { "shown", dialog.Shown },         { "optionRows", dialog.OptionRows },
                                             { "optionLines", dialog.OptionLines },
                                             { "result", sDialogResult } };
                  }() },
                { "savePending", SaveManager::Instance->IsSaving() },
                { "messageState", Message_GetState(&gPlayState->msgCtx) }, { "presentationPending", OotmmItemPresentation_IsPending() ? 1 : 0 }, { "backgroundShown", OotmmItemPresentation_DebugBackgroundShown() }, { "hiddenGlitterDraws", OotmmDrops_DebugHiddenGlitterDraws() },
                { "pauseState", gPlayState->pauseCtx.state },
                { "pausePage", gPlayState->pauseCtx.pageIndex },
                { "infoMenu", nlohmann::json({ OotmmPauseInfo_Screen(), OotmmPauseInfo_Cursor() }) },
                { "bottles", nlohmann::json({ OotmmBottles_ReadSlot(0), OotmmBottles_ReadSlot(1), OotmmBottles_ReadSlot(2),
                                              OotmmBottles_ReadSlot(3), OotmmBottles_ReadSlot(4), OotmmBottles_ReadSlot(5) }) },
                { "chateau", OotmmBottles_ChateauActive() },
                { "cvars", [] {
                     nlohmann::json values = nlohmann::json::object();
                     const char* names = std::getenv("OOTMM_RUNTIME_CVARS");
                     std::string list = names != nullptr ? names : "";
                     size_t start = 0;
                     while (start < list.size()) {
                         size_t end = list.find(',', start);
                         end = end == std::string::npos ? list.size() : end;
                         const std::string name = list.substr(start, end - start);
                         if (!name.empty()) {
                             values[name] = CVarGetInteger(name.c_str(), -1);
                         }
                         start = end + 1;
                     }
                     return values;
                 }() },
                { "rutoLetter", OotmmItemApply_LedgerGet("native:rutoLetterUsed") },
                { "fishing", []() {
                     s32 values[6] = { 0 };
                     if (gPlayState->sceneNum == SCENE_FISHING_POND) {
                         Fishing_DebugState(values);
                     }
                     return nlohmann::json(values);
                 }() },
                { "go2", []() {
                     nlohmann::json out = nlohmann::json::array();
                     for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr;
                          actor = actor->next) {
                         if (actor->id == ACTOR_EN_GO2 && (actor->params & 0x1F) == 0) {
                             auto* goron = reinterpret_cast<EnGo2*>(actor);
                             out.push_back({ goron->collider.base.acFlags, goron->collider.info.bumperFlags,
                                             goron->interactInfo.talkState, goron->waypoint });
                         }
                     }
                     return out;
                 }() },
                { "zTarget", gSaveContext.zTargetSetting },
                { "pauseCursor", gPlayState->pauseCtx.cursorPoint },
                { "pauseCursorSlot", gPlayState->pauseCtx.cursorSlot },
                { "pauseCursorItem", gPlayState->pauseCtx.cursorItem },
                { "pauseCursorSpecial", gPlayState->pauseCtx.cursorSpecialPos },
                { "cButtonItems", gSaveContext.equips.buttonItems },
                { "equipment", gSaveContext.equips.equipment }, { "ownedEquipment", gSaveContext.inventory.equipment }, { "bgsFlag", gSaveContext.bgsFlag }, { "swordHealth", gSaveContext.swordHealth }, { "mmSongs", OotmmSongs_OwnedMask() }, { "ocarinaPlayback", [] { u8 st = 0, note = 0, pos = 0; Audio_OcaGetPlaybackDebug(&st, &note, &pos); return nlohmann::json({ st, note, pos }); }() },
                { "x", player->actor.world.pos.x }, { "y", player->actor.world.pos.y },
                { "z", player->actor.world.pos.z }, { "yaw", player->actor.shape.rot.y },
                { "stateFlags1", player->stateFlags1 }, { "stateFlags2", player->stateFlags2 }, { "message", gPlayState->msgCtx.textId },
                { "itemAction", player->itemAction }, { "heldItemAction", player->heldItemAction },
                { "leftHandType", player->leftHandType }, { "itemCast", player->unk_860 }, { "spinCharge", player->unk_858 },
                { "ocarinaStaff", gPlayState->msgCtx.msgMode != MSGMODE_OCARINA_PLAYING
                                      ? nlohmann::json(nullptr)
                                      : nlohmann::json({ gPlayState->msgCtx.ocarinaStaff->noteIdx,
                                                         gPlayState->msgCtx.ocarinaStaff->state,
                                                         gPlayState->msgCtx.ocarinaStaff->pos }) }, { "currentMask", player->currentMask }, { "sandstorm", gPlayState->envCtx.sandstormState },
                { "minigameState", gSaveContext.minigameState }, { "shootingGallery", gPlayState->shootingGalleryStatus },
                { "transition", gPlayState->transitionTrigger }, { "transitionMode", gPlayState->transitionMode },
                { "heldActor", player->heldActor != nullptr ? player->heldActor->id : -1 }, { "animFrame", player->skelAnime.curFrame }, { "animSpeed", player->skelAnime.playSpeed }, { "csAction", player->csAction },
                { "bItem", gSaveContext.equips.buttonItems[0] },
                { "cItems", nlohmann::json::array({ gSaveContext.equips.buttonItems[1], gSaveContext.equips.buttonItems[2], gSaveContext.equips.buttonItems[3] }) },
                { "dpadItems", nlohmann::json::array({ gSaveContext.equips.buttonItems[4], gSaveContext.equips.buttonItems[5], gSaveContext.equips.buttonItems[6], gSaveContext.equips.buttonItems[7] }) },
                { "ocarinaMode", gPlayState->msgCtx.ocarinaMode },
                { "zTarget", gSaveContext.zTargetSetting },
                { "ambience", gPlayState->sequenceCtx.natureAmbienceId },
                { "bootsEquip", static_cast<int>(CUR_EQUIP_VALUE(EQUIP_TYPE_BOOTS)) },
                { "tokenDbg", tokenDbg },
                { "polyDbg", polyDbg },
                { "inCs", Player_InCsMode(gPlayState) },
                { "playerCyl", { GET_PLAYER(gPlayState)->cylinder.dim.pos.y, GET_PLAYER(gPlayState)->cylinder.dim.height,
                                 GET_PLAYER(gPlayState)->cylinder.dim.yShift, GET_PLAYER(gPlayState)->cylinder.base.ocFlags1 } },
                { "buttons", gPlayState->state.input[0].cur.button }, { "checks", checks },
                { "actors", actors }, { "setupActors", [] {
                     nlohmann::json list = nlohmann::json::array();
                     const int count = std::abs(gPlayState->numSetupActors);
                     for (int i = 0; gPlayState->setupActorList != nullptr && i < count; i++) {
                         const ActorEntry& entry = gPlayState->setupActorList[i];
                         list.push_back({ { "id", entry.id }, { "params", entry.params }, { "x", entry.pos.x },
                                          { "y", entry.pos.y }, { "z", entry.pos.z }, { "rz", entry.rot.z } });
                     }
                     return list;
                 }() }, { "cameraYaw", Camera_GetInputDirYaw(GET_ACTIVE_CAM(gPlayState)) },
                { "focusYaw", player->actor.focus.rot.y }, { "focusPitch", player->actor.focus.rot.x },
                { "dayTime", gSaveContext.dayTime }, { "sunsSongState", gSaveContext.sunsSongState }, { "fanfare", { { "resolved", OotmmItemPresentation_DebugLastFanfare() }, { "seq", func_800FA0B4(SEQ_PLAYER_FANFARE) }, { "font", gAudioContext.seqPlayers[SEQ_PLAYER_FANFARE].defaultFont } } },
                { "pvp", { { "enabled", gOotmmPvpEnabled }, { "sent", gOotmmPvpHitsSent },
                           { "received", gOotmmPvpHitsReceived }, { "applied", gOotmmPvpHitsApplied } } },
                { "qol", { { "fairySpotPads", gOotmmFairySpotPads }, { "potRolling", gOotmmPotRolling } } },
                { "sunPosition", { { "x", gPlayState->envCtx.sunPos.x },
                                     { "y", gPlayState->envCtx.sunPos.y }, { "z", gPlayState->envCtx.sunPos.z } } },
                { "playerBody7", { { "x", player->bodyPartsPos[7].x },
                                     { "y", player->bodyPartsPos[7].y }, { "z", player->bodyPartsPos[7].z } } },
                { "sunHitboxes", sunHitboxes }, { "ganonTail", ganonTail }, { "kingDodongo", kingDodongo }, { "sfx", recentSfx }, { "bomMmId", gOotmmBomMmId }, { "clearTagId", gOotmmClearTagId },
                { "triforce", { { "count", OotmmTriforce_Count() }, { "goal", OotmmTriforce_Goal() }, { "displayMax", OotmmTriforce_DisplayMax() }, { "won", OotmmTriforce_HasWon() } } }
            };
            if (gPlayState->pauseCtx.state == 7) {
                observation["pauseMode"] = gPlayState->pauseCtx.unk_1EC;
                observation["pauseChoice"] = gPlayState->pauseCtx.promptChoice;
            }
            const auto chest = OotmmItemPresentation_DebugChestState();
            const Camera* camera = GET_ACTIVE_CAM(gPlayState);
            observation["chestPresentation"] = {
                { "attempts", chest.Attempts }, { "handoffs", chest.Handoffs },
                { "matchedQueue", chest.MatchedQueue }, { "actorId", chest.ActorId },
                { "actorScale", chest.ActorScale }, { "large", chest.Large },
                { "delivered", chest.Delivered },
                { "nativeItemAction", player->actionFunc == Player_Action_8084E6D4 },
                { "actionVar1", player->av1.actionVar1 }, { "actionVar2", player->av2.actionVar2 },
                { "getItemId", player->getItemId }, { "getItemFrom", player->getItemEntry.getItemFrom },
                { "cameraSetting", camera->setting }, { "cameraMode", camera->mode },
                { "cameraData2", camera->data2 }
            };
            Ship::OotmmRuntimeTest::Get().Observe(std::move(observation));
        });
    }
#endif
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) {
        sDropOptions.clear();
        sDropSelection = 0;
        sNpcOptions.clear();
        sNpcSelection = 0;
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) {
        sDebugChests.clear();
        OotmmCsmc_ClearChestOverrides();
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_CHEST, {
        EnBox* chest = va_arg(args, EnBox*);
        Player* player = GET_PLAYER(gPlayState);
        if (player != nullptr && IsDebugChest(&chest->dyna.actor)) {
            OotmmChecks_OpenChestSilently(gPlayState, player);
            *should = false;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneFlagSet>(
        [](int16_t sceneNum, int16_t flagType, int16_t flag) {
            if (flagType != FLAG_SCENE_TREASURE || gPlayState == nullptr) {
                return;
            }
            for (const auto& [chest, usedFlag] : sDebugChests) {
                if (usedFlag == flag) {
                    gPlayState->actorCtx.flags.chest &= ~(1 << flag);
                    return;
                }
            }
        });
}
