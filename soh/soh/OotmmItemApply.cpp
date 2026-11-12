#include "OotmmItemApply.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmBottles.h"
#include "OotmmCustomItems.h"
#include "OotmmDungeons.h"
#include "OotmmRegionState.h"
#include "OotmmFaroresWind.h"
#include "OotmmIpc.h"
#include "OotmmItemPresentation.h"
#include "OotmmShops.h"
#include "OotmmSession.h"
#include "OotmmSilverRupees.h"
#include "OotmmTriforce.h"

#include <libultraship/bridge/OotmmAppliedLedger.h>
#include <libultraship/bridge/OotmmItemGrant.h>
#include <libultraship/bridge/OotmmHealth.h>
#include <libultraship/bridge/OotmmMagic.h>
#include <libultraship/bridge/OotmmBombchu.h>
#include <libultraship/bridge/OotmmSharedFamilies.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"

extern PlayState* gPlayState;
}

namespace {

const std::unordered_map<std::string, uint8_t> kNativeItems = {
#include "OotmmNativeItems.inc"
};

struct Ladder {
    std::string_view Suffix;
    std::vector<uint8_t> Tiers;
};

const std::vector<Ladder> kLadders = {
    { "SWORD", { ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER, ITEM_SWORD_KNIFE, ITEM_SWORD_BGS } },
    { "SWORD_GORON", { ITEM_SWORD_KNIFE, ITEM_SWORD_BGS } },
    { "SHIELD", { ITEM_SHIELD_DEKU, ITEM_SHIELD_HYLIAN, ITEM_SHIELD_MIRROR } },
    { "BOMB_BAG", { ITEM_BOMB_BAG_20, ITEM_BOMB_BAG_30, ITEM_BOMB_BAG_40 } },
    { "BOW", { ITEM_BOW, ITEM_QUIVER_40, ITEM_QUIVER_50 } },
    { "SLINGSHOT", { ITEM_SLINGSHOT, ITEM_BULLET_BAG_40, ITEM_BULLET_BAG_50 } },
    { "HOOKSHOT", { ITEM_HOOKSHOT, ITEM_LONGSHOT } },
    { "OCARINA", { ITEM_OCARINA_FAIRY, ITEM_OCARINA_TIME } },
    { "STRENGTH", { ITEM_BRACELET, ITEM_GAUNTLETS_SILVER, ITEM_GAUNTLETS_GOLD } },
    { "NUT_UPGRADE", { ITEM_NUT_UPGRADE_30, ITEM_NUT_UPGRADE_40 } },
    { "STICK_UPGRADE", { ITEM_STICK_UPGRADE_20, ITEM_STICK_UPGRADE_30 } },
};

const std::vector<std::pair<std::string_view, uint8_t>> kAmmoSlots = {
    { "ARROW", ITEM_BOW },    { "BOMBCHU", ITEM_BOMBCHU }, { "BOMB", ITEM_BOMB },
    { "NUT", ITEM_NUT },      { "STICK", ITEM_STICK },     { "SEED", ITEM_SLINGSHOT },
};

const std::unordered_map<std::string, uint8_t> kQuestFlags = {
    { "OOT_SONG_TIME", QUEST_SONG_TIME },
    { "OOT_SONG_EPONA", QUEST_SONG_EPONA },
    { "OOT_SONG_SUN", QUEST_SONG_SUN },
    { "OOT_SONG_SARIA", QUEST_SONG_SARIA },
    { "OOT_SONG_STORMS", QUEST_SONG_STORMS },
    { "OOT_SONG_ZELDA", QUEST_SONG_LULLABY },
    { "OOT_SONG_TP_FOREST", QUEST_SONG_MINUET },
    { "OOT_SONG_TP_FIRE", QUEST_SONG_BOLERO },
    { "OOT_SONG_TP_WATER", QUEST_SONG_SERENADE },
    { "OOT_SONG_TP_SPIRIT", QUEST_SONG_REQUIEM },
    { "OOT_SONG_TP_SHADOW", QUEST_SONG_NOCTURNE },
    { "OOT_SONG_TP_LIGHT", QUEST_SONG_PRELUDE },
    { "SHARED_SONG_TIME", QUEST_SONG_TIME },
    { "SHARED_SONG_EPONA", QUEST_SONG_EPONA },
    { "SHARED_SONG_STORMS", QUEST_SONG_STORMS },
    { "SHARED_SONG_ZELDA", QUEST_SONG_LULLABY },
    { "SHARED_SONG_SARIA", QUEST_SONG_SARIA },
    { "SHARED_SONG_SUN", QUEST_SONG_SUN },
    { "SHARED_SONG_TP_FOREST", QUEST_SONG_MINUET },
    { "SHARED_SONG_TP_FIRE", QUEST_SONG_BOLERO },
    { "SHARED_SONG_TP_WATER", QUEST_SONG_SERENADE },
    { "SHARED_SONG_TP_SPIRIT", QUEST_SONG_REQUIEM },
    { "SHARED_SONG_TP_SHADOW", QUEST_SONG_NOCTURNE },
    { "SHARED_SONG_TP_LIGHT", QUEST_SONG_PRELUDE },
    { "OOT_MEDALLION_FOREST", QUEST_MEDALLION_FOREST },
    { "OOT_MEDALLION_FIRE", QUEST_MEDALLION_FIRE },
    { "OOT_MEDALLION_WATER", QUEST_MEDALLION_WATER },
    { "OOT_MEDALLION_SPIRIT", QUEST_MEDALLION_SPIRIT },
    { "OOT_MEDALLION_SHADOW", QUEST_MEDALLION_SHADOW },
    { "OOT_MEDALLION_LIGHT", QUEST_MEDALLION_LIGHT },
    { "OOT_STONE_EMERALD", QUEST_KOKIRI_EMERALD },
    { "OOT_STONE_RUBY", QUEST_GORON_RUBY },
    { "OOT_STONE_SAPPHIRE", QUEST_ZORA_SAPPHIRE },
    { "OOT_STONE_OF_AGONY", QUEST_STONE_OF_AGONY },
    { "SHARED_STONE_OF_AGONY", QUEST_STONE_OF_AGONY },
};

struct SongNotes {
    const char* SongItemId;
    const char* SongName;
    const char* NoteItemId;
    const char* SharedNoteItemId;
    uint32_t Needed;
};

constexpr SongNotes kSongNotes[] = {
    { "OOT_SONG_TP_FOREST", "Minuet of Forest", "OOT_SONG_NOTE_TP_FOREST", "SHARED_SONG_NOTE_TP_FOREST", 6 },
    { "OOT_SONG_TP_FIRE", "Bolero of Fire", "OOT_SONG_NOTE_TP_FIRE", "SHARED_SONG_NOTE_TP_FIRE", 8 },
    { "OOT_SONG_TP_WATER", "Serenade of Water", "OOT_SONG_NOTE_TP_WATER", "SHARED_SONG_NOTE_TP_WATER", 5 },
    { "OOT_SONG_TP_SPIRIT", "Requiem of Spirit", "OOT_SONG_NOTE_TP_SPIRIT", "SHARED_SONG_NOTE_TP_SPIRIT", 6 },
    { "OOT_SONG_TP_SHADOW", "Nocturne of Shadow", "OOT_SONG_NOTE_TP_SHADOW", "SHARED_SONG_NOTE_TP_SHADOW", 7 },
    { "OOT_SONG_TP_LIGHT", "Prelude of Light", "OOT_SONG_NOTE_TP_LIGHT", "SHARED_SONG_NOTE_TP_LIGHT", 6 },
    { "OOT_SONG_ZELDA", "Zelda's Lullaby", "OOT_SONG_NOTE_ZELDA", "SHARED_SONG_NOTE_ZELDA", 6 },
    { "OOT_SONG_EPONA", "Epona's Song", "OOT_SONG_NOTE_EPONA", "SHARED_SONG_NOTE_EPONA", 6 },
    { "OOT_SONG_SARIA", "Saria's Song", "OOT_SONG_NOTE_SARIA", "SHARED_SONG_NOTE_SARIA", 6 },
    { "OOT_SONG_SUN", "Sun's Song", "OOT_SONG_NOTE_SUN", "SHARED_SONG_NOTE_SUN", 6 },
    { "OOT_SONG_TIME", "Song of Time", "OOT_SONG_NOTE_TIME", "SHARED_SONG_NOTE_TIME", 6 },
    { "OOT_SONG_STORMS", "Song of Storms", "OOT_SONG_NOTE_STORMS", "SHARED_SONG_NOTE_STORMS", 6 },
};


int DungeonSceneOf(std::string_view suffix) {
    const size_t underscore = suffix.rfind('_');
    const std::string_view code =
        underscore == std::string_view::npos ? suffix : suffix.substr(underscore + 1);
    return OotmmDungeons_SceneForCode(std::string(code).c_str());
}

Ship::OotmmAppliedLedger sLedger;
Ship::OotmmSharedResourceState sSharedResources;

uint32_t CollectedSmallKeys(int scene) {
    const char* code = OotmmDungeons_CodeForScene(scene);
    const uint32_t max = static_cast<uint32_t>(OotmmDungeons_MaxSmallKeys(scene));
    if (code == nullptr || max == 0) {
        return 0;
    }
    if (sLedger.Applied("OOT_SKELETON_KEY") != 0 || sLedger.Applied("SHARED_SKELETON_KEY") != 0 ||
        sLedger.Applied(std::string("OOT_KEY_RING_") + code) != 0) {
        return max;
    }
    return std::min(max, sLedger.Applied(std::string("OOT_SMALL_KEY_") + code));
}

void ApplySmallKeyItem(const std::string& itemId, uint32_t count) {
    constexpr int kScenes = ARRAY_COUNT(gSaveContext.inventory.dungeonKeys);
    uint32_t before[kScenes];
    for (int scene = 0; scene < kScenes; ++scene) {
        before[scene] = CollectedSmallKeys(scene);
    }
    sLedger.SetApplied(itemId, count);
    for (int scene = 0; scene < kScenes; ++scene) {
        const uint32_t after = CollectedSmallKeys(scene);
        if (after > before[scene]) {
            s8& held = gSaveContext.inventory.dungeonKeys[scene];
            held = static_cast<s8>((held < 0 ? 0 : held) + (after - before[scene]));
        }
    }
}

bool FamilyEnabled(const Ship::OotmmSharedFamily& family) {
    if (!family.EnabledValue.empty()) {
        return OotmmSession_GetState().GetStringSetting(std::string(family.Setting), "") == family.EnabledValue;
    }
    return OotmmSession_GetState().GetBoolSetting(std::string(family.Setting), false);
}

bool AnySharedFamily() {
    for (const auto& family : Ship::OotmmSharedFamilies()) {
        if (FamilyEnabled(family)) {
            return true;
        }
    }
    return false;
}

bool SharedReward(const std::string& id) {
    for (const auto& family : Ship::OotmmSharedFamilies()) {
        if (FamilyEnabled(family) && family.IsReward(id)) {
            return true;
        }
    }
    return false;
}

bool LoadSharedResources(const nlohmann::json& saved) {
    if (!AnySharedFamily()) {
        return true;
    }
    const auto& boot = OotmmSession_GetState().GetBootConfig();
    return sSharedResources.Load(boot.SharedResources, saved, boot.SaveId, gSaveContext.fileNum);
}
std::map<std::string, std::string> sUnhandled;
uint64_t sAppliedRevision = 0;
bool sRetryMagicApplication = false;
bool sLedgerLoaded = false;
bool sLedgerValid = true;
std::unordered_map<int, nlohmann::json> sSaveSnapshots;
std::unordered_map<int, nlohmann::json> sSharedSaveSnapshots;

std::string LedgerPathFor(int fileNum) {
    const auto& state = OotmmSession_GetState();
    return state.GetBootConfig().StatePath + ".applied.oot-" + state.GetNativeSaveTag() + "." +
           std::to_string(fileNum);
}

std::string LedgerPath() { return LedgerPathFor(gSaveContext.fileNum); }

std::string LegacyLedgerPath() {
    return OotmmSession_GetState().GetBootConfig().StatePath + ".applied." +
           std::to_string(gSaveContext.fileNum);
}

void EnsureLedger() {
    if (sLedgerLoaded) {
        return;
    }
    const bool legacyValid = sLedger.LoadLegacy(LedgerPath(), LegacyLedgerPath());
    sLedgerValid = legacyValid;
    const auto saved = sSaveSnapshots.find(gSaveContext.fileNum);
    if (saved != sSaveSnapshots.end()) {
        sLedgerValid = sLedger.RestoreSnapshot(saved->second);
        if (!sLedgerValid) {
            SPDLOG_ERROR("[OoTMM] Invalid applied-item snapshot; delivery is suspended");
        }
    }
    if (sLedgerValid && !LoadSharedResources(sSharedSaveSnapshots[gSaveContext.fileNum])) {
        sLedgerValid = false;
        SPDLOG_ERROR("[OoTMM] Invalid shared-resource snapshot");
    }
    if (sLedgerValid) {
        const auto health = Ship::ResolveOotmmSavedHealth(
            gSaveContext.healthCapacity, gSaveContext.health, gSaveContext.inventory.questItems >> 28,
            uint64_t(sLedger.Applied("OOT_HEART_CONTAINER")) + sLedger.Applied("SHARED_HEART_CONTAINER"),
            uint64_t(sLedger.Applied("OOT_HEART_PIECE")) + sLedger.Applied("SHARED_HEART_PIECE"));
        if (!health) {
            sLedgerValid = false;
            SPDLOG_ERROR("[OoTMM] Applied health rewards exceed native save bounds; delivery is suspended");
        } else {
            gSaveContext.healthCapacity = health->Capacity;
            gSaveContext.health += health->Healing;
            gSaveContext.inventory.questItems = (gSaveContext.inventory.questItems & 0x0fffffffU) |
                (uint32_t(health->Pieces) << 28);
        }
    }
    sLedgerLoaded = true;
}

const Ladder* FindLadder(std::string_view suffix) {
    for (const auto& ladder : kLadders) {
        if (ladder.Suffix == suffix) {
            return &ladder;
        }
    }
    return nullptr;
}

std::vector<uint8_t> LadderTiers(std::string_view suffix) {
    if (suffix == "WALLET") {
        std::vector<uint8_t> tiers;
        if (OotmmSession_GetState().GetBoolSetting("childWallets", false)) {
            tiers.push_back(ITEM_NONE);
        }
        tiers.push_back(ITEM_WALLET_ADULT);
        tiers.push_back(ITEM_WALLET_GIANT);
        return tiers;
    }
    if (suffix == "SCALE") {
        std::vector<uint8_t> tiers;
        if (OotmmSession_GetState().GetBoolSetting("bronzeScale", false)) {
            tiers.push_back(ITEM_NONE);
        }
        tiers.push_back(ITEM_SCALE_SILVER);
        tiers.push_back(ITEM_SCALE_GOLDEN);
        return tiers;
    }
    if (suffix == "SWORD" && OotmmSession_GetState().GetBoolSetting("extraChildSwordsOot", false)) {
        return { ITEM_SWORD_KOKIRI, ITEM_NONE, ITEM_NONE };
    }
    if (const Ladder* ladder = FindLadder(suffix)) {
        return ladder->Tiers;
    }
    return {};
}

uint32_t TrailingNumber(std::string_view suffix, uint32_t fallback) {
    const size_t underscore = suffix.rfind('_');
    if (underscore == std::string_view::npos) {
        return fallback;
    }
    uint32_t value = 0;
    for (const char c : suffix.substr(underscore + 1)) {
        if (c < '0' || c > '9') {
            return fallback;
        }
        value = value * 10 + static_cast<uint32_t>(c - '0');
    }
    return value == 0 ? fallback : value;
}

int16_t RupeeValue(std::string_view suffix) {
    if (suffix.find("BLUE") != std::string_view::npos) return 5;
    if (suffix.find("RED") != std::string_view::npos) return 20;
    if (suffix.find("PURPLE") != std::string_view::npos) return 50;
    if (suffix.find("GOLD") != std::string_view::npos) return 200;
    if (suffix.find("HUGE") != std::string_view::npos) return 200;
    if (suffix.find("RAINBOW") != std::string_view::npos) return 999;
    return 1;
}

uint32_t SilverRupeesPerUnit(const std::string& slot) {
    if (slot == "RUPEE_MAGICAL") {
        uint32_t total = 0;
        for (const std::string_view puzzle : Ship::OotmmSilverRupees::All()) {
            total += static_cast<uint32_t>(OotmmSilverRupeesRequired(std::string(puzzle).c_str()));
        }
        return total;
    }
    if (slot.starts_with("POUCH_SILVER_")) {
        return static_cast<uint32_t>(
            OotmmSilverRupeesRequired(slot.c_str() + sizeof("POUCH_SILVER_") - 1));
    }
    if (slot.starts_with("RUPEE_SILVER_")) {
        return 1;
    }
    return 0;
}

bool GrantsNativeItem(const std::string& itemId, uint32_t count, uint8_t item) {
    for (const auto& op : Ship::OotmmItemGrant::Resolve(itemId, count)) {
        if (op.Kind != Ship::OotmmGrantKind::Equipment ||
            !Ship::OotmmItemGrant::AppliesTo(op, Ship::OotmmGrantGame::Oot)) {
            continue;
        }
        const std::vector<uint8_t> tiers = LadderTiers(op.Slot);
        if (tiers.empty()) {
            const auto found = kNativeItems.find(itemId);
            if (found != kNativeItems.end() && found->second == item) {
                return true;
            }
            continue;
        }
        const size_t reached = std::min<size_t>(op.Amount, tiers.size());
        for (size_t tier = 0; tier < reached; tier++) {
            if (tiers[tier] == item) {
                return true;
            }
        }
    }
    return false;
}

void ApplyOnce(PlayState* play, const std::string& key, uint8_t item) {
    sLedger.SetApplied(key, 1);
    if (item == ITEM_NONE) {
        return;
    }
    if (item == ITEM_SWORD_KNIFE || item == ITEM_SWORD_BGS) {
        Item_Give(play, ITEM_SWORD_BGS);
        if (item == ITEM_SWORD_BGS) {
            gSaveContext.bgsFlag = 1;
        }
    } else {
        Item_Give(play, item);
    }
    SPDLOG_INFO("[OoTMM grant] gave {} (item {})", key, static_cast<int>(item));
}

bool IsNewBottle(const std::string& itemId) {
    return itemId.find("_BOTTLE_") != std::string::npos || itemId.find("_BOTTLED_") != std::string::npos;
}

bool IsBottleRefill(const std::string& itemId) {
    static constexpr std::array<std::string_view, 11> kRefills = {
        "POTION_RED", "POTION_GREEN", "POTION_BLUE", "FAIRY", "POE", "BIG_POE",
        "BLUE_FIRE",  "MILK",         "CHATEAU",     "BUGS",  "FISH",
    };
    if (!itemId.starts_with("OOT_") && !itemId.starts_with("SHARED_")) {
        return false;
    }
    const std::string_view suffix = Ship::OotmmItemGrant::SuffixOf(itemId);
    return std::find(kRefills.begin(), kRefills.end(), suffix) != kRefills.end();
}

void FillBottles(const std::string& itemId, uint32_t already, uint32_t count) {
    const auto found = kNativeItems.find(itemId);
    if (found == kNativeItems.end()) {
        sUnhandled[itemId] = "BOTTLE (no native item)";
        return;
    }
    for (uint32_t unit = already; unit < count; unit++) {
        const bool filled = OotmmBottles_FillEmpty(found->second) != 0;
        sLedger.SetApplied(itemId, unit + 1);
        SPDLOG_INFO("[OoTMM grant] {} {} (item {})", filled ? "filled a bottle with" : "had no empty bottle for",
                    itemId, static_cast<int>(found->second));
    }
}

void GiveNewBottles(const std::string& itemId, uint32_t already, uint32_t count) {
    const auto found = kNativeItems.find(itemId);
    if (found == kNativeItems.end()) {
        sUnhandled[itemId] = "BOTTLE (no native item)";
        return;
    }
    for (uint32_t unit = already; unit < count; unit++) {
        const uint8_t item = unit > 0 && Ship::OotmmIsProgressiveBottle(itemId) ? ITEM_BOTTLE : found->second;
        if (!OotmmBottles_GiveNew(item)) {
            sUnhandled[itemId] = "No free bottle slot";
            return;
        }
        sLedger.SetApplied(itemId, unit + 1);
        SPDLOG_INFO("[OoTMM grant] gave {} (new bottle, item {})", itemId, static_cast<int>(item));
    }
}

bool IsConsumable(uint8_t item) {
    return item == ITEM_STICK || item == ITEM_NUT || item == ITEM_BOMB;
}

void GiveUnits(PlayState* play, const std::string& itemId, uint8_t item, uint32_t already, uint32_t count) {
    for (uint32_t unit = already; unit < count && unit - already < 255; unit++) {
        Item_Give(play, item);
    }
    sLedger.SetApplied(itemId, count);
    SPDLOG_INFO("[OoTMM grant] gave {} x{} (item {})", itemId, count - already, static_cast<int>(item));
}

bool IsCustomItemBacked(const std::string& itemId) {
    return OotmmCustomItems_IdForItemId(itemId.c_str()) != ITEM_NONE;
}

const std::vector<uint8_t> kChildTradeItems = {
    ITEM_WEIRD_EGG,  ITEM_CHICKEN,    ITEM_LETTER_ZELDA, ITEM_MASK_KEATON,
    ITEM_MASK_SKULL, ITEM_MASK_SPOOKY, ITEM_MASK_BUNNY,  ITEM_MASK_GORON,
    ITEM_MASK_ZORA,  ITEM_MASK_GERUDO, ITEM_MASK_TRUTH,
};

const std::vector<uint8_t> kAdultTradeItems = {
    ITEM_POCKET_EGG, ITEM_POCKET_CUCCO, ITEM_COJIRO,       ITEM_ODD_MUSHROOM,
    ITEM_ODD_POTION, ITEM_SAW,          ITEM_SWORD_BROKEN, ITEM_PRESCRIPTION,
    ITEM_FROG,       ITEM_EYEDROPS,     ITEM_CLAIM_CHECK,
};

const std::vector<uint8_t>* TradeItemsForSlot(uint8_t slot) {
    if (slot == SLOT_TRADE_CHILD) {
        return &kChildTradeItems;
    }
    if (slot == SLOT_TRADE_ADULT) {
        return &kAdultTradeItems;
    }
    return nullptr;
}

std::string NativeTradeKey(uint8_t item) {
    return "OOT_TRADE_NATIVE:" + std::to_string(item);
}

bool IsTradeGrantOp(const Ship::OotmmGrantOp& op) {
    switch (op.Kind) {
        case Ship::OotmmGrantKind::Equipment:
        case Ship::OotmmGrantKind::Upgrade:
        case Ship::OotmmGrantKind::Mask:
        case Ship::OotmmGrantKind::Bottle:
            return Ship::OotmmItemGrant::AppliesTo(op, Ship::OotmmGrantGame::Oot) &&
                   LadderTiers(op.Slot).empty();
        default:
            return false;
    }
}

bool IsGrantedTradeItem(const std::string& itemId, uint32_t count, uint8_t item) {
    if (count == 0 || IsCustomItemBacked(itemId)) {
        return false;
    }
    const auto found = kNativeItems.find(itemId);
    if (found == kNativeItems.end() || found->second != item) {
        return false;
    }
    for (const auto& op : Ship::OotmmItemGrant::Resolve(itemId, count)) {
        if (IsTradeGrantOp(op)) {
            return true;
        }
    }
    return false;
}

std::string SpentTradeKey(uint8_t item) {
    return "OOT_TRADE_SPENT:" + std::to_string(item);
}

uint32_t GrantedTradeCount(uint8_t item) {
    uint32_t total = 0;
    for (const auto& [itemId, count] : OotmmIpc_GetInventory().GetValues()) {
        if (IsGrantedTradeItem(itemId, count, item)) {
            total += count;
        }
    }
    return total;
}

std::vector<uint8_t> GrantedTradeItems(const std::vector<uint8_t>& candidates) {
    std::vector<uint8_t> granted;
    for (const uint8_t item : candidates) {
        if (GrantedTradeCount(item) > sLedger.Applied(SpentTradeKey(item))) {
            granted.push_back(item);
        }
    }
    return granted;
}

std::vector<uint8_t> OwnedTradeItems(uint8_t slot) {
    const std::vector<uint8_t>* candidates = TradeItemsForSlot(slot);
    if (candidates == nullptr) {
        return {};
    }
    EnsureLedger();
    const uint8_t current = gSaveContext.inventory.items[slot];
    if (std::find(candidates->begin(), candidates->end(), current) != candidates->end() &&
        !sLedger.Has(NativeTradeKey(current))) {
        sLedger.SetApplied(NativeTradeKey(current), 1);
        sLedger.Save(LedgerPath());
    }

    const std::vector<uint8_t> granted = GrantedTradeItems(*candidates);
    std::vector<uint8_t> owned;
    for (const uint8_t item : *candidates) {
        if (item == current || sLedger.Has(NativeTradeKey(item)) ||
            std::find(granted.begin(), granted.end(), item) != granted.end()) {
            owned.push_back(item);
        }
    }
    if (const uint8_t bottle = OotmmBottles_TradeSlotBottle(slot); bottle != ITEM_NONE) {
        owned.push_back(bottle);
    }
    return owned;
}

uint32_t BombchuBagTier() {
    const uint64_t bags = uint64_t(sLedger.Applied("OOT_BOMBCHU_BAG")) +
                          sLedger.Applied("SHARED_BOMBCHU_BAG");
    return static_cast<uint32_t>(std::min<uint64_t>(3,
        std::max<uint64_t>(bags, sLedger.Applied("native:bombchuBagFirst"))));
}

int32_t BombchuCapacity() {
    const auto tier = BombchuBagTier();
    return Ship::ResolveOotmmBombchuCapacity(
        OotmmSession_GetState().GetStringSetting("bombchuBehaviorOot", "free"),
        CUR_UPG_VALUE(UPG_BOMB_BAG), tier).value_or(0);
}

uint32_t ReadSharedValue(std::string_view key) {
    if (OotmmFaroresWind_IsSharedKey(key)) {
        return OotmmFaroresWind_ReadShared(key);
    }
    if (const auto trap = Ship::OotmmTrapForSetting(key); !trap.empty()) {
        return sLedger.Applied(std::string(trap));
    }
    if (key == "bossClears") {
        return sLedger.Applied("native:bossClears");
    }
    if (const int bottle = Ship::OotmmBottleIndex(key); bottle >= 0) {
        return OotmmBottles_ReadSlot(bottle);
    }
    if (key == "rutoLetter") {
        return sLedger.Applied("native:rutoLetterUsed");
    }
    if (key == "chateau") {
        return sLedger.Applied("native:chateau");
    }
    const auto& inventory = gSaveContext.inventory;
    if (key == "bombchu") {
        return static_cast<uint32_t>(inventory.ammo[SLOT_BOMBCHU]);
    }
    if (key == "bombchuOwned") {
        return inventory.items[SLOT_BOMBCHU] == ITEM_BOMBCHU ? 1u : 0u;
    }
    if (key == "bombchuBagFirst") {
        return sLedger.Applied("native:bombchuBagFirst");
    }
    if (key == "rupees") {
        return static_cast<uint32_t>(gSaveContext.rupees);
    }
    if (key == "health") {
        return static_cast<uint32_t>(gSaveContext.health);
    }
    if (key == "healthCapacity") {
        return static_cast<uint32_t>(gSaveContext.healthCapacity);
    }
    if (key == "heartPieces") {
        return (inventory.questItems >> 28) & 0xF;
    }
    if (key == "magic") {
        return static_cast<uint32_t>(gSaveContext.magic);
    }
    if (key == "arrows") {
        return static_cast<uint32_t>(inventory.ammo[SLOT_BOW]);
    }
    if (key == "bombs") {
        return static_cast<uint32_t>(inventory.ammo[SLOT_BOMB]);
    }
    if (key == "nuts") {
        return static_cast<uint32_t>(inventory.ammo[SLOT_NUT]);
    }
    if (key == "sticks") {
        return static_cast<uint32_t>(inventory.ammo[SLOT_STICK]);
    }
    if (key == "seeds") {
        return static_cast<uint32_t>(inventory.ammo[SLOT_SLINGSHOT]);
    }
    if (key == "keg") {
        return static_cast<uint32_t>(OotmmCustomItems_KegAmmo());
    }
    return 0;
}

constexpr int kLastOotBossDungeon = 7;
constexpr int kLastBossDungeon = 12;

void ApplyBossClears(uint32_t mask) {
    uint32_t applied = sLedger.Applied("native:bossClearsApplied");
    for (int dungeon = 0; dungeon <= kLastOotBossDungeon; ++dungeon) {
        const uint32_t bit = 1u << dungeon;
        if ((mask & bit) && !(applied & bit)) {
            OotmmRegionState_ApplyBossClear(dungeon);
            applied |= bit;
        }
    }
    sLedger.SetApplied("native:bossClearsApplied", applied);
}

uint32_t SharedValueLimit(std::string_view key) {
    if (OotmmFaroresWind_IsSharedKey(key)) {
        return 2;
    }
    if (Ship::OotmmBottleIndex(key) >= 0) {
        return static_cast<uint32_t>(Ship::OotmmBottleContent::Count) - 1;
    }
    if (key == "rutoLetter") {
        return Ship::kOotmmRutoLetterUsedOot | Ship::kOotmmRutoLetterUsedMm;
    }
    if (key == "chateau") {
        return 1;
    }
    if (!Ship::OotmmTrapForSetting(key).empty()) {
        return 65535;
    }
    if (key == "bossClears") {
        return 0xFFFF;
    }
    if (key == "bombchuOwned" || key == "bombchuBagFirst" || key == "keg") {
        return 1;
    }
    if (key == "rupees") {
        return 9999;
    }
    if (key == "health" || key == "healthCapacity") {
        return 0x140;
    }
    if (key == "heartPieces") {
        return 3;
    }
    if (key == "magic") {
        return 0x60;
    }
    return 99;
}

void WriteSharedValue(std::string_view key, uint32_t value) {
    auto& inventory = gSaveContext.inventory;
    if (OotmmFaroresWind_IsSharedKey(key)) {
        OotmmFaroresWind_WriteShared(key, value);
    } else if (const auto trap = Ship::OotmmTrapForSetting(key); !trap.empty()) {
        sLedger.SetApplied(std::string(trap), std::max(sLedger.Applied(std::string(trap)), value));
    } else if (key == "bossClears") {
        const uint32_t mask = sLedger.Applied("native:bossClears") | value;
        sLedger.SetApplied("native:bossClears", mask);
        ApplyBossClears(mask);
    } else if (const int bottle = Ship::OotmmBottleIndex(key); bottle >= 0) {
        OotmmBottles_WriteSlot(bottle, value);
    } else if (key == "rutoLetter") {
        sLedger.SetApplied("native:rutoLetterUsed", sLedger.Applied("native:rutoLetterUsed") | value);
    } else if (key == "chateau") {
        sLedger.SetApplied("native:chateau", value);
    } else if (key == "bombchu") {
        inventory.ammo[SLOT_BOMBCHU] = static_cast<int8_t>(value);
    } else if (key == "bombchuOwned") {
        inventory.items[SLOT_BOMBCHU] = value ? ITEM_BOMBCHU : ITEM_NONE;
    } else if (key == "bombchuBagFirst") {
        sLedger.SetApplied("native:bombchuBagFirst", value);
    } else if (key == "rupees") {
        gSaveContext.rupees = static_cast<s16>(value);
        gSaveContext.rupeeAccumulator = 0;
    } else if (key == "health") {
        gSaveContext.health = static_cast<s16>(value);
        gSaveContext.healthAccumulator = 0;
    } else if (key == "healthCapacity") {
        gSaveContext.healthCapacity = static_cast<s16>(value);
    } else if (key == "heartPieces") {
        inventory.questItems = (inventory.questItems & 0x0FFFFFFF) | (value << 28);
    } else if (key == "magic") {
        gSaveContext.magic = static_cast<s8>(value);
    } else if (key == "arrows") {
        inventory.ammo[SLOT_BOW] = static_cast<int8_t>(value);
    } else if (key == "bombs") {
        inventory.ammo[SLOT_BOMB] = static_cast<int8_t>(value);
    } else if (key == "nuts") {
        inventory.ammo[SLOT_NUT] = static_cast<int8_t>(value);
    } else if (key == "sticks") {
        inventory.ammo[SLOT_STICK] = static_cast<int8_t>(value);
    } else if (key == "seeds") {
        inventory.ammo[SLOT_SLINGSHOT] = static_cast<int8_t>(value);
    } else if (key == "keg") {
        OotmmCustomItems_SetKegAmmo(static_cast<int>(value));
    }
}

bool RestoreSharedResources() {
    if (!AnySharedFamily() || !sSharedResources.NeedsRestore()) {
        return true;
    }
    const auto& snapshot = *sSharedResources.Current();
    for (const auto& [key, value] : snapshot.Values) {
        if (value > SharedValueLimit(key)) {
            return false;
        }
    }
    for (const auto& [id, count] : snapshot.Applied) {
        if (!SharedReward(id) || OotmmIpc_GetInventory().Count(id) < count) {
            return false;
        }
    }
    std::vector<std::string> previous;
    for (const auto& [id, count] : sLedger.GetValues()) {
        if (SharedReward(id)) {
            previous.push_back(id);
        }
    }
    for (const auto& id : previous) {
        sLedger.Erase(id);
    }
    for (const auto& [id, count] : snapshot.Applied) {
        sLedger.SetApplied(id, count);
    }
    for (const auto& family : Ship::OotmmSharedFamilies()) {
        if (!FamilyEnabled(family)) {
            continue;
        }
        for (const auto key : family.Values) {
            if (const auto value = snapshot.Values.find(std::string(key)); value != snapshot.Values.end()) {
                WriteSharedValue(key, value->second);
            }
        }
    }
    sSharedResources.Restored();
    return true;
}

struct WalletState {
    bool HasWallet = true;
    uint8_t Level = 0;
    bool Bottomless = false;
    uint32_t Owned = 0;
};

WalletState sWallet;

WalletState ComputeWallet() {
    const auto& state = OotmmSession_GetState();
    const auto& inventory = OotmmIpc_GetInventory();
    WalletState wallet;
    wallet.Owned = inventory.Count("OOT_WALLET") + inventory.Count("SHARED_WALLET");
    uint32_t upgrades = wallet.Owned;
    if (state.GetBoolSetting("childWallets", false)) {
        wallet.HasWallet = upgrades > 0;
        upgrades = upgrades > 0 ? upgrades - 1 : 0;
    }
    const bool colossal = state.GetBoolSetting("colossalWallets", false);
    const uint32_t maxLevel = colossal ? 3 : 2;
    wallet.Level = static_cast<uint8_t>(std::min(upgrades, maxLevel));
    wallet.Bottomless = colossal && state.GetBoolSetting("bottomlessWallets", false) && upgrades > maxLevel;
    return wallet;
}

void RefreshWallet() {
    sWallet = ComputeWallet();
    gUpgradeCapacities[UPG_WALLET][0] = sWallet.HasWallet ? 99 : 0;
    gUpgradeCapacities[UPG_WALLET][3] = sWallet.Bottomless ? 9999 : 999;
    if (CUR_UPG_VALUE(UPG_WALLET) != sWallet.Level) {
        Inventory_ChangeUpgrade(UPG_WALLET, sWallet.Level);
    }
    if (gSaveContext.rupees > CUR_CAPACITY(UPG_WALLET)) {
        gSaveContext.rupees = CUR_CAPACITY(UPG_WALLET);
    }
}

void ApplyWallet() {
    RefreshWallet();
    const bool fill = OotmmSession_GetState().GetBoolSetting("fillWallets", false);
    for (uint32_t tier = 0; tier < sWallet.Owned; tier++) {
        const std::string key = "WALLET_TIER:" + std::to_string(tier);
        if (sLedger.Has(key)) {
            continue;
        }
        sLedger.SetApplied(key, 1);
        if (fill) {
            Rupees_ChangeBy(static_cast<s16>(CUR_CAPACITY(UPG_WALLET)));
        }
    }
}

void ApplyOne(PlayState* play, const std::string& itemId, uint32_t count) {
    if (count == 0 || IsCustomItemBacked(itemId)) {
        return;
    }
    if (IsBottleRefill(itemId)) {
        FillBottles(itemId, sLedger.Applied(itemId), count);
        return;
    }
    for (const auto& op : Ship::OotmmItemGrant::Resolve(itemId, count)) {
        if (!Ship::OotmmItemGrant::AppliesTo(op, Ship::OotmmGrantGame::Oot)) {
            continue;
        }
        const uint32_t already = sLedger.Applied(itemId);
        switch (op.Kind) {
            case Ship::OotmmGrantKind::Equipment:
            case Ship::OotmmGrantKind::Upgrade:
            case Ship::OotmmGrantKind::Mask:
            case Ship::OotmmGrantKind::Bottle: {
                if (op.Slot == "WALLET") {
                    ApplyWallet();
                    break;
                }
                if (op.Slot == "BOMBCHU_BAG") {
                    gSaveContext.inventory.items[SLOT_BOMBCHU] = ITEM_BOMBCHU;
                    if (count > already) {
                        sLedger.SetApplied(itemId, count);
                        gSaveContext.inventory.ammo[SLOT_BOMBCHU] = BombchuCapacity();
                    }
                    break;
                }
                if (op.Slot == "DEFENSE_UPGRADE") {
                    if (count > already) {
                        const auto health = Ship::ResolveOotmmHealth(
                            gSaveContext.healthCapacity, gSaveContext.health, gSaveContext.inventory.questItems >> 28, 0, 0, 20);
                        if (!health) {
                            sUnhandled[itemId] = "Invalid native health for defense reward";
                            break;
                        }
                        gSaveContext.isDoubleDefenseAcquired = 1;
                        gSaveContext.inventory.defenseHearts = 20;
                        Health_ChangeBy(play, health->Healing);
                        sLedger.SetApplied(itemId, count);
                    }
                    break;
                }
                if (IsNewBottle(itemId)) {
                    GiveNewBottles(itemId, already, count);
                    break;
                }
                const std::vector<uint8_t> tiers = LadderTiers(op.Slot);
                if (!tiers.empty()) {
                    if (op.Slot == "BOW") {
                        INV_CONTENT(ITEM_BOW) = ITEM_BOW;
                    } else if (op.Slot == "SLINGSHOT") {
                        INV_CONTENT(ITEM_SLINGSHOT) = ITEM_SLINGSHOT;
                    }
                    const size_t tier = std::min<size_t>(op.Amount, tiers.size()) - 1;
                    const std::string key = itemId + ":" + std::to_string(tier);
                    if (!sLedger.Has(key)) {
                        ApplyOnce(play, key, tiers[tier]);
                    }
                    break;
                }
                if (const auto found = kNativeItems.find(itemId);
                    found != kNativeItems.end() && IsConsumable(found->second)) {
                    if (count > already) {
                        GiveUnits(play, itemId, found->second, already, count);
                    }
                    break;
                }
                if (const auto found = kNativeItems.find(itemId);
                    found != kNativeItems.end() && found->second >= ITEM_SHIELD_DEKU &&
                    found->second <= ITEM_SHIELD_MIRROR) {
                    if (count > already) {
                        ApplyOnce(play, itemId, found->second);
                        sLedger.SetApplied(itemId, count);
                    }
                    break;
                }
                if (already == 0) {
                    const auto found = kNativeItems.find(itemId);
                    if (found != kNativeItems.end()) {
                        ApplyOnce(play, itemId, found->second);
                    } else {
                        sUnhandled[itemId] = op.Slot + " (no native item)";
                    }
                }
                break;
            }
            case Ship::OotmmGrantKind::Rupees: {
                if (count > already) {
                    Rupees_ChangeBy(static_cast<int16_t>((count - already) * RupeeValue(op.Slot)));
                    sLedger.SetApplied(itemId, count);
                }
                break;
            }
            case Ship::OotmmGrantKind::Ammo: {
                if (count <= already) {
                    break;
                }
                if (op.Slot == "BOMBCHU" || op.Slot.starts_with("BOMBCHU_")) {
                    if (OotmmSession_GetState().GetStringSetting("bombchuBehaviorOot", "free") == "bagFirst") {
                        sLedger.SetApplied("native:bombchuBagFirst", 1);
                    }
                    const auto capacity = BombchuCapacity();
                    if (capacity > 0) {
                        gSaveContext.inventory.items[SLOT_BOMBCHU] = ITEM_BOMBCHU;
                        const uint64_t amount = uint64_t(count - already) * TrailingNumber(op.Slot, 1);
                        gSaveContext.inventory.ammo[SLOT_BOMBCHU] = static_cast<int8_t>(std::min<uint64_t>(
                            capacity, std::max<int32_t>(0, gSaveContext.inventory.ammo[SLOT_BOMBCHU]) + amount));
                    }
                    sLedger.SetApplied(itemId, count);
                    break;
                }
                for (const auto& [name, slot] : kAmmoSlots) {
                    if (op.Slot.rfind(name, 0) == 0) {
                        const uint32_t per = TrailingNumber(op.Slot, 1);
                        Inventory_ChangeAmmo(slot, static_cast<int16_t>((count - already) * per));
                        sLedger.SetApplied(itemId, count);
                        break;
                    }
                }
                break;
            }
            case Ship::OotmmGrantKind::BigFairy: {
                if (count > already) {
                    if (gSaveContext.isMagicAcquired && gSaveContext.magicState != MAGIC_STATE_IDLE) {
                        sRetryMagicApplication = true;
                        break;
                    }
                    Health_ChangeBy(play, static_cast<s16>((count - already) * 8 * 0x10));
                    if (gSaveContext.isMagicAcquired) {
                        Magic_Fill(play);
                    }
                    sLedger.SetApplied(itemId, count);
                }
                break;
            }
            case Ship::OotmmGrantKind::Health: {
                if (count > already) {
                    const uint32_t delta = count - already;
                    const bool container = op.Slot == "HEART_CONTAINER";
                    const bool piece = op.Slot == "HEART_PIECE";
                    const auto result = Ship::ResolveOotmmHealth(
                        gSaveContext.healthCapacity, gSaveContext.health, gSaveContext.inventory.questItems >> 28,
                        container ? delta : 0, piece ? delta : 0, (!container && !piece) ? delta : 0);
                    if (!result) {
                        sUnhandled[itemId] = "Health reward exceeds native save bounds";
                        break;
                    }
                    gSaveContext.healthCapacity = result->Capacity;
                    gSaveContext.inventory.questItems = (gSaveContext.inventory.questItems & 0x0fffffffU) |
                        (uint32_t(result->Pieces) << 28);
                    if (container) {
                        gSaveContext.ship.stats.heartContainers = static_cast<uint8_t>(std::min<uint64_t>(255,
                            uint64_t(gSaveContext.ship.stats.heartContainers) + delta));
                    } else if (piece) {
                        gSaveContext.ship.stats.heartPieces = static_cast<uint8_t>(std::min<uint64_t>(255,
                            uint64_t(gSaveContext.ship.stats.heartPieces) + delta));
                    }
                    Health_ChangeBy(play, result->Healing);
                    sLedger.SetApplied(itemId, count);
                }
                break;
            }
            case Ship::OotmmGrantKind::Magic: {
                if (op.Slot == "MAGIC_UPGRADE") {
                    const auto& inventory = OotmmIpc_GetInventory();
                    const auto upgrade = Ship::ResolveOotmmMagicUpgrade(
                        inventory.Count("OOT_MAGIC_UPGRADE"), inventory.Count("SHARED_MAGIC_UPGRADE"),
                        count, already, gSaveContext.isMagicAcquired, gSaveContext.isDoubleMagicAcquired);
                    if (upgrade.Refill) {
                        gSaveContext.isMagicAcquired = 1;
                        gSaveContext.isDoubleMagicAcquired = upgrade.Tier >= 2;
                        gSaveContext.magicLevel = 0;
                        gSaveContext.magicFillTarget = upgrade.Tier * MAGIC_NORMAL_METER;
                        sLedger.SetApplied(itemId, count);
                    }
                    break;
                }
                if (count > already) {
                    const auto& inventory = OotmmIpc_GetInventory();
                    const bool ownsMagic = inventory.Count("OOT_MAGIC_UPGRADE") != 0 ||
                                           inventory.Count("SHARED_MAGIC_UPGRADE") != 0;
                    if ((!gSaveContext.isMagicAcquired && ownsMagic) ||
                        (gSaveContext.isMagicAcquired &&
                         (gSaveContext.magicLevel == 0 ||
                          gSaveContext.magicCapacity != (gSaveContext.isDoubleMagicAcquired ?
                                                             MAGIC_DOUBLE_METER : MAGIC_NORMAL_METER) ||
                          gSaveContext.magicState == MAGIC_STATE_STEP_CAPACITY ||
                          gSaveContext.magicState == MAGIC_STATE_FILL ||
                          gSaveContext.magicState == MAGIC_STATE_ADD))) {
                        sRetryMagicApplication = true;
                        break;
                    }
                    const auto amount = Ship::ResolveOotmmMagicJarAmount(
                        Ship::OotmmGrantGame::Oot, OotmmSession_GetState().GetBoolSetting("sharedMagic", false),
                        op.Slot == "MAGIC_JAR_LARGE", count - already);
                    if (gSaveContext.isMagicAcquired && !Magic_RequestChange(play, amount, MAGIC_ADD)) {
                        sRetryMagicApplication = true;
                        break;
                    }
                    sLedger.SetApplied(itemId, count);
                }
                break;
            }
            case Ship::OotmmGrantKind::Song:
            case Ship::OotmmGrantKind::QuestFlag: {
                if (already != 0) {
                    break;
                }
                const auto found = kQuestFlags.find(itemId);
                if (found != kQuestFlags.end()) {
                    gSaveContext.inventory.questItems |= gBitFlags[found->second];
                    sLedger.SetApplied(itemId, 1);
                }
                break;
            }
            case Ship::OotmmGrantKind::DungeonItem: {
                const int scene = op.Slot == "BOSS_KEY_GANON" ? SCENE_GANONS_TOWER : DungeonSceneOf(op.Slot);
                if (scene < 0 ||
                    scene >= static_cast<int>(ARRAY_COUNT(gSaveContext.inventory.dungeonItems))) {
                    break;
                }
                if (op.Slot.rfind("SMALL_KEY", 0) == 0) {
                    if (count > already) {
                        ApplySmallKeyItem(itemId, count);
                    }
                    break;
                }
                if (already != 0) {
                    break;
                }
                const uint8_t bit = op.Slot.rfind("MAP", 0) == 0        ? DUNGEON_MAP
                                    : op.Slot.rfind("COMPASS", 0) == 0  ? DUNGEON_COMPASS
                                    : op.Slot.rfind("BOSS_KEY", 0) == 0 ? DUNGEON_KEY_BOSS
                                                                        : 0xFF;
                if (bit != 0xFF) {
                    gSaveContext.inventory.dungeonItems[scene] |= gBitFlags[bit];
                    sLedger.SetApplied(itemId, 1);
                }
                break;
            }
            case Ship::OotmmGrantKind::SmallKeyRing:
            case Ship::OotmmGrantKind::SkeletonKey: {
                if (already == 0) {
                    ApplySmallKeyItem(itemId, 1);
                }
                break;
            }
            case Ship::OotmmGrantKind::Token: {
                if (count <= already) {
                    break;
                }
                if (op.Slot == "PLATINUM_TOKEN") {
                    gSaveContext.inventory.gsTokens = 100;
                } else {
                    gSaveContext.inventory.questItems |= gBitFlags[QUEST_SKULL_TOKEN];
                    gSaveContext.inventory.gsTokens =
                        static_cast<s16>(std::min<int>(100, gSaveContext.inventory.gsTokens + (count - already)));
                }
                sLedger.SetApplied(itemId, count);
                break;
            }
            case Ship::OotmmGrantKind::SilverRupee: {
                if (count <= already) {
                    break;
                }
                const uint32_t payout = (count - already) * SilverRupeesPerUnit(op.Slot) * 5;
                if (payout != 0) {
                    Rupees_ChangeBy(static_cast<int16_t>(std::min<uint32_t>(payout, 999)));
                }
                sLedger.SetApplied(itemId, count);
                OotmmSilverRupeesSyncFlags(play);
                break;
            }
            default:
                if (!Ship::OotmmItemGrant::IsInventoryBacked(op.Kind)) {
                    sUnhandled[itemId] = op.Slot;
                }
                break;
        }
    }
}

constexpr u32 kTrapBlockingStates = PLAYER_STATE1_HOOKSHOT_FALLING | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD |
                                    PLAYER_STATE1_READY_TO_FIRE | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_ON_HORSE;

void HurtForTrap(Player* player, s32 response, f32 speed, f32 yVelocity, s16 yaw) {
    player->actor.colChkInfo.damage = 0;
    func_80837C0C(gPlayState, player, response, speed, yVelocity, yaw, 20);
}

bool ApplyTrap(size_t type, Player* player) {
    switch (type) {
        case 0:
            HurtForTrap(player, 3, 0, 0, 0);
            return true;
        case 1:
            if (player->currentTunic == 2) {
                return true;
            }
            HurtForTrap(player, 0, 0, 0, 0);
            player->bodyIsBurning = true;
            for (int part = 0; part < PLAYER_BODYPART_MAX; part++) {
                player->bodyFlameTimers[part] = Rand_S16Offset(0, 200);
            }
            return true;
        case 2:
            HurtForTrap(player, 4, 0, 0, 0);
            return true;
        case 3:
            gSaveContext.health = 1;
            HurtForTrap(player, 0, 0, 0, 0);
            return true;
        case 4:
            gSaveContext.magic = 0;
            HurtForTrap(player, 0, 0, 0, 0);
            return true;
        case 5:
            HurtForTrap(player, 1, Rand_ZeroFloat(20.0f), Rand_ZeroFloat(15.0f), static_cast<s16>(Rand_Next() & 0xFFFF));
            return true;
        case 6:
            Rupees_ChangeBy(-10);
            return true;
        default:
            return false;
    }
}

void UpdateTraps() {
    if (gPlayState == nullptr || !OotmmSession_IsActive() || !sLedgerValid ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || gPlayState->pauseCtx.state != 0 ||
        gPlayState->msgCtx.msgMode != MSGMODE_NONE || OotmmItemPresentation_IsPending() ||
        OotmmShops_InTransaction(gPlayState)) {
        return;
    }
    const auto& inventory = OotmmIpc_GetInventory();
    if (inventory.GetRevision() != sAppliedRevision) {
        return;
    }
    for (size_t type = 0; type < Ship::kOotmmTraps.size(); type++) {
        const std::string id(Ship::kOotmmTraps[type].first);
        const uint32_t applied = sLedger.Applied(id);
        if (inventory.Count(id) <= applied) {
            continue;
        }
        Player* player = GET_PLAYER(gPlayState);
        if (player == nullptr || (player->stateFlags1 & kTrapBlockingStates) ||
            (player->stateFlags2 & PLAYER_STATE2_CRAWLING) || player->invincibilityTimer != 0) {
            return;
        }
        if (ApplyTrap(type, player)) {
            sLedger.SetApplied(id, applied + 1);
            sLedger.Save(LedgerPath());
        }
        return;
    }
}

void ApplyCompletedSongNotes(const Ship::OotmmInventory& inventory) {
    for (const auto& song : kSongNotes) {
        const uint8_t quest = kQuestFlags.at(song.SongItemId);
        const std::string key = std::string("@notes:") + song.SongItemId;
        if (sLedger.Applied(key) != 0 || CHECK_QUEST_ITEM(quest) ||
            inventory.Count(song.NoteItemId) + inventory.Count(song.SharedNoteItemId) < song.Needed) {
            continue;
        }
        gSaveContext.inventory.questItems |= gBitFlags[quest];
        sLedger.SetApplied(key, 1);
        Ship::GameIpcItemPresentation presentation;
        presentation.ItemId = song.SongItemId;
        presentation.ItemName = song.SongName;
        presentation.ItemGame = "oot";
        presentation.DestinationGame = "oot";
        OotmmItemPresentation_Queue(presentation);
    }
}

} // namespace

extern "C" int32_t OotmmItemApply_FoundMasterSword(void) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    for (const auto& [itemId, count] : OotmmIpc_GetInventory().GetValues()) {
        if (GrantsNativeItem(itemId, count, ITEM_SWORD_MASTER)) {
            return 1;
        }
    }
    return 0;
}

extern "C" int32_t OotmmItemApply_TradeCycleNeighbors(uint8_t slot, uint8_t* outLeft,
                                                      uint8_t* outRight) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const std::vector<uint8_t> owned = OwnedTradeItems(slot);
    if (owned.size() < 2) {
        return 0;
    }
    const uint8_t current = gSaveContext.inventory.items[slot];
    const auto position = std::find(owned.begin(), owned.end(), current);
    if (position == owned.end()) {
        *outLeft = owned.back();
        *outRight = owned.front();
    } else {
        const size_t index = static_cast<size_t>(position - owned.begin());
        *outLeft = owned[(index + owned.size() - 1) % owned.size()];
        *outRight = owned[(index + 1) % owned.size()];
    }
    return 1;
}

extern "C" int32_t OotmmItemApply_TradeSlotCandidates(uint8_t slot, uint8_t* outItems,
                                                      int32_t capacity) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const std::vector<uint8_t>* candidates = TradeItemsForSlot(slot);
    if (candidates == nullptr) {
        return 0;
    }
    int32_t count = 0;
    for (const uint8_t item : *candidates) {
        if (count >= capacity) {
            break;
        }
        outItems[count++] = item;
    }
    return count;
}

extern "C" int32_t OotmmItemApply_OwnedTradeItems(uint8_t slot, uint8_t* outItems,
                                                  int32_t capacity) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const std::vector<uint8_t> owned = OwnedTradeItems(slot);
    int32_t count = 0;
    for (const uint8_t item : owned) {
        if (count >= capacity) {
            break;
        }
        outItems[count++] = item;
    }
    return count;
}

extern "C" void OotmmItemApply_SetTradeItemOwned(uint8_t slot, uint8_t item, int32_t owned) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    const std::vector<uint8_t>* candidates = TradeItemsForSlot(slot);
    if (candidates == nullptr ||
        std::find(candidates->begin(), candidates->end(), item) == candidates->end()) {
        return;
    }
    EnsureLedger();
    if (owned) {
        sLedger.SetApplied(NativeTradeKey(item), 1);
        sLedger.Save(LedgerPath());
        const uint8_t current = gSaveContext.inventory.items[slot];
        if (std::find(candidates->begin(), candidates->end(), current) == candidates->end()) {
            gSaveContext.inventory.items[slot] = item;
        }
        return;
    }

    sLedger.Erase(NativeTradeKey(item));
    sLedger.Save(LedgerPath());
    for (const auto& [itemId, count] : OotmmIpc_GetInventory().GetValues()) {
        if (IsGrantedTradeItem(itemId, count, item)) {
            OotmmIpc_SetDebugItemValue(itemId, 0);
        }
    }
    if (gSaveContext.inventory.items[slot] == item) {
        gSaveContext.inventory.items[slot] = ITEM_NONE;
        for (const uint8_t replacement : OwnedTradeItems(slot)) {
            if (replacement != item) {
                gSaveContext.inventory.items[slot] = replacement;
                break;
            }
        }
    }
}

extern "C" void OotmmItemApply_ConsumeTradeItem(uint8_t slot, uint8_t item) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    const std::vector<uint8_t>* candidates = TradeItemsForSlot(slot);
    if (candidates == nullptr ||
        std::find(candidates->begin(), candidates->end(), item) == candidates->end()) {
        return;
    }
    EnsureLedger();
    sLedger.Erase(NativeTradeKey(item));
    if (const uint32_t granted = GrantedTradeCount(item); granted > 0) {
        sLedger.SetApplied(SpentTradeKey(item), granted);
    }
    sLedger.Save(LedgerPath());

    for (size_t button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
        if (gSaveContext.equips.buttonItems[button] == item) {
            gSaveContext.equips.buttonItems[button] = ITEM_NONE;
            gSaveContext.equips.cButtonSlots[button - 1] = SLOT_NONE;
        }
    }
    if (gSaveContext.inventory.items[slot] == item) {
        gSaveContext.inventory.items[slot] = ITEM_NONE;
        for (const uint8_t replacement : OwnedTradeItems(slot)) {
            if (replacement != item) {
                gSaveContext.inventory.items[slot] = replacement;
                break;
            }
        }
    }
}

extern "C" int32_t OotmmItemApply_BombchuCapacity(void) {
    if (!OotmmSession_IsActive()) {
        return 50;
    }
    EnsureLedger();
    return sLedgerValid ? BombchuCapacity() : 0;
}

extern "C" int32_t OotmmItemApply_SelectBombDrop(int32_t bombDrop, int32_t bombchuDrop) {
    if (!OotmmSession_IsActive()) {
        return bombDrop;
    }
    EnsureLedger();
    if (!sLedgerValid) {
        return -1;
    }
    switch (Ship::ResolveOotmmBombDrop(CUR_UPG_VALUE(UPG_BOMB_BAG) > 0, BombchuBagTier() > 0,
        std::max<int32_t>(0, gSaveContext.inventory.ammo[SLOT_BOMB]),
        std::max<int32_t>(0, gSaveContext.inventory.ammo[SLOT_BOMBCHU]))) {
        case Ship::OotmmBombDrop::None: return -1;
        case Ship::OotmmBombDrop::Bomb: return bombDrop;
        case Ship::OotmmBombDrop::Bombchu: return bombchuDrop;
        case Ship::OotmmBombDrop::Random: return Rand_ZeroOne() < 0.5f ? bombchuDrop : bombDrop;
    }
    return -1;
}

extern "C" void OotmmItemApply_AddBombchuAmmo(int32_t amount) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    const int32_t capacity = OotmmItemApply_BombchuCapacity();
    if (capacity == 0 || !sLedgerValid) {
        return;
    }
    if (amount > 0) {
        gSaveContext.inventory.items[SLOT_BOMBCHU] = ITEM_BOMBCHU;
    }
    gSaveContext.inventory.ammo[SLOT_BOMBCHU] = static_cast<int8_t>(std::clamp<int64_t>(
        int64_t(gSaveContext.inventory.ammo[SLOT_BOMBCHU]) + amount, 0, capacity));
}

void OotmmItemApply_ResetLedgerForNewSave() {
    if (!OotmmSession_IsActive()) {
        return;
    }
    sLedger.Reset();
    sSaveSnapshots[gSaveContext.fileNum] = nlohmann::json::object();
    sSharedSaveSnapshots[gSaveContext.fileNum] = nullptr;
    sLedgerValid = LoadSharedResources(nullptr);
    sLedger.Save(LedgerPath());
    sLedgerLoaded = true;
    sAppliedRevision = 0;
}

static void RepairFirstBulletBagGrant(PlayState* play) {
    auto& items = gSaveContext.inventory.items;
    for (size_t slot = 0; slot < ARRAY_COUNT(gSaveContext.inventory.items); slot++) {
        if (items[slot] == ITEM_BULLET_BAG_30) {
            const bool ownsSticks = slot == SLOT_STICK && (CUR_UPG_VALUE(UPG_STICKS) != 0 || AMMO(ITEM_STICK) > 0);
            items[slot] = ownsSticks ? ITEM_STICK : ITEM_NONE;
        }
    }
    for (size_t button = 1; button < ARRAY_COUNT(gSaveContext.equips.buttonItems); button++) {
        if (gSaveContext.equips.buttonItems[button] != ITEM_BULLET_BAG_30) {
            continue;
        }
        const uint8_t slot = gSaveContext.equips.cButtonSlots[button - 1];
        const uint8_t restored = slot < ARRAY_COUNT(gSaveContext.inventory.items) ? items[slot] : ITEM_NONE;
        gSaveContext.equips.buttonItems[button] = restored;
        if (restored == ITEM_NONE) {
            gSaveContext.equips.cButtonSlots[button - 1] = SLOT_NONE;
        } else {
            Interface_LoadItemIcon1(play, static_cast<u16>(button));
        }
    }
    if (INV_CONTENT(ITEM_SLINGSHOT) == ITEM_SLINGSHOT && CUR_UPG_VALUE(UPG_BULLET_BAG) == 0) {
        Inventory_ChangeUpgrade(UPG_BULLET_BAG, 1);
        AMMO(ITEM_SLINGSHOT) = CUR_CAPACITY(UPG_BULLET_BAG);
    }
}

void OotmmItemApply_Reconcile() {
    if (gPlayState == nullptr || !OotmmSession_IsActive()) {
        return;
    }
    RepairFirstBulletBagGrant(gPlayState);
    const auto& inventory = OotmmIpc_GetInventory();
    if (inventory.GetRevision() == sAppliedRevision) {
        return;
    }
    EnsureLedger();
    if (!sLedgerValid) {
        return;
    }
    sRetryMagicApplication = false;
    RefreshWallet();
    for (const auto& [itemId, count] : inventory.GetValues()) {
        if (Ship::OotmmItemGrant::SuffixOf(itemId) == "BOMB_BAG" ||
            (Ship::OotmmItemGrant::GameOf(itemId) == Ship::OotmmGrantGame::Shared && !SharedReward(itemId))) {
            ApplyOne(gPlayState, itemId, count);
        }
    }
    if (!RestoreSharedResources()) {
        return;
    }
    size_t owned = 0;
    for (const auto& [itemId, count] : inventory.GetValues()) {
        if (count > 0) {
            owned++;
        }
        ApplyOne(gPlayState, itemId, count);
    }
    ApplyCompletedSongNotes(inventory);
    if (!sRetryMagicApplication) {
        sAppliedRevision = inventory.GetRevision();
    }
    SPDLOG_INFO("[OoTMM grant] revision {}: {} entries, {} owned", inventory.GetRevision(),
                inventory.GetValues().size(), owned);
    sLedger.Save(LedgerPath());
    if (!sUnhandled.empty()) {
        SPDLOG_WARN("[OoTMM grant] {} owned items have no grant path:", sUnhandled.size());
        for (const auto& [id, slot] : sUnhandled) {
            SPDLOG_WARN("[OoTMM grant]   {} ({})", id, slot);
        }
        sUnhandled.clear();
    }
}

int16_t OotmmWallet_ScaleRupees(int16_t amount) {
    if (amount <= 0 || !OotmmSession_IsActive() || !OotmmSession_GetState().GetBoolSetting("rupeeScaling", false)) {
        return amount;
    }
    const int extra = (CUR_UPG_VALUE(UPG_WALLET) == 3 ? 1 : 0) + (sWallet.Bottomless ? 1 : 0);
    int32_t value = amount;
    if (extra >= 1) {
        value *= 2;
    }
    if (extra >= 2) {
        value *= 10;
    }
    return static_cast<int16_t>(std::min(value, 9999));
}

int32_t OotmmWallet_HasWallet(void) {
    return !OotmmSession_IsActive() || sWallet.HasWallet;
}

int32_t OotmmWallet_Bottomless(void) {
    return OotmmSession_IsActive() && sWallet.Bottomless;
}

uint32_t OotmmItemApply_LedgerGet(const std::string& key) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    EnsureLedger();
    return sLedger.Applied(key);
}

void OotmmItemApply_LedgerSet(const std::string& key, uint32_t value) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    EnsureLedger();
    sLedger.SetApplied(key, value);
    sLedger.Save(LedgerPath());
}


bool OotmmItemApply_CaptureSharedSnapshot(nlohmann::json& snapshot) {
    snapshot = nullptr;
    if (!OotmmSession_IsActive() || !AnySharedFamily()) {
        return true;
    }
    EnsureLedger();
    if (!sLedgerValid) {
        return false;
    }
    const auto& boot = OotmmSession_GetState().GetBootConfig();
    std::map<std::string, uint32_t> applied;
    for (const auto& [id, count] : sLedger.GetValues()) {
        if (SharedReward(id)) {
            applied.emplace(id, count);
        }
    }
    std::map<std::string, uint32_t> values;
    for (const auto& family : Ship::OotmmSharedFamilies()) {
        if (FamilyEnabled(family)) {
            for (const auto key : family.Values) {
                values.emplace(std::string(key), ReadSharedValue(key));
            }
        }
    }
    return sSharedResources.Capture(boot.SaveId, gSaveContext.fileNum, "oot", values, applied, snapshot);
}

bool OotmmItemApply_CaptureSnapshot(nlohmann::json& snapshot) {
    if (!OotmmSession_IsActive()) {
        snapshot = nullptr;
        return true;
    }
    EnsureLedger();
    if (!sLedgerValid) {
        return false;
    }
    snapshot = sLedger.GetValues();
    return true;
}

nlohmann::json OotmmItemApply_LegacySnapshot(int fileNum) {
    Ship::OotmmAppliedLedger ledger;
    if (!ledger.LoadLegacy(LedgerPathFor(fileNum),
            OotmmSession_GetState().GetBootConfig().StatePath + ".applied." + std::to_string(fileNum))) {
        return nullptr;
    }
    return ledger.GetValues();
}

void OotmmItemApply_SetSaveSnapshot(int fileNum, const nlohmann::json& snapshot, const nlohmann::json& shared) {
    sSaveSnapshots[fileNum] = snapshot;
    sSharedSaveSnapshots[fileNum] = shared;
    sLedgerLoaded = false;
    sAppliedRevision = 0;
}

void OotmmItemApply_RecordBossClear(int dungeonId) {
    if (dungeonId < 0 || dungeonId > kLastBossDungeon) {
        return;
    }
    EnsureLedger();
    const uint32_t bit = 1u << dungeonId;
    sLedger.SetApplied("native:bossClears", sLedger.Applied("native:bossClears") | bit);
    if (dungeonId <= kLastOotBossDungeon) {
        sLedger.SetApplied("native:bossClearsApplied", sLedger.Applied("native:bossClearsApplied") | bit);
    }
}

nlohmann::json OotmmItemApply_TrapsApplied() {
    nlohmann::json applied = nlohmann::json::object();
    if (sLedgerValid) {
        for (const auto& [id, setting] : Ship::kOotmmTraps) {
            applied[std::string(id)] = sLedger.Applied(std::string(id));
        }
    }
    return applied;
}

void OotmmItemApply_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        OotmmItemApply_Reconcile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        []() { OotmmTriforce_Update(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(UpdateTraps);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) {
        sAppliedRevision = 0;
        sLedgerLoaded = false;
    });
}
