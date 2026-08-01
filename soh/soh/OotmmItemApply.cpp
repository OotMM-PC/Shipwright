#include "OotmmItemApply.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmCustomItems.h"
#include "OotmmDungeons.h"
#include "OotmmIpc.h"
#include "OotmmSession.h"
#include "OotmmSilverRupees.h"
#include "OotmmTriforce.h"

#include <libultraship/bridge/OotmmAppliedLedger.h>
#include <libultraship/bridge/OotmmItemGrant.h>
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
    { "BOW", { ITEM_QUIVER_30, ITEM_QUIVER_40, ITEM_QUIVER_50 } },
    { "SLINGSHOT", { ITEM_BULLET_BAG_30, ITEM_BULLET_BAG_40, ITEM_BULLET_BAG_50 } },
    { "HOOKSHOT", { ITEM_HOOKSHOT, ITEM_LONGSHOT } },
    { "OCARINA", { ITEM_OCARINA_FAIRY, ITEM_OCARINA_TIME } },
    { "STRENGTH", { ITEM_BRACELET, ITEM_GAUNTLETS_SILVER, ITEM_GAUNTLETS_GOLD } },
    { "NUT_UPGRADE", { ITEM_NUT_UPGRADE_30, ITEM_NUT_UPGRADE_40 } },
    { "STICK_UPGRADE", { ITEM_STICK_UPGRADE_20, ITEM_STICK_UPGRADE_30 } },
};

const std::vector<std::pair<std::string_view, uint8_t>> kAmmoSlots = {
    { "ARROWS", ITEM_BOW },    { "BOMBS", ITEM_BOMB },   { "BOMBCHU", ITEM_BOMBCHU },
    { "NUTS", ITEM_NUT },      { "STICKS", ITEM_STICK }, { "SEEDS", ITEM_SLINGSHOT },
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

int DungeonSceneOf(std::string_view suffix) {
    const size_t underscore = suffix.rfind('_');
    const std::string_view code =
        underscore == std::string_view::npos ? suffix : suffix.substr(underscore + 1);
    return OotmmDungeons_SceneForCode(std::string(code).c_str());
}

Ship::OotmmAppliedLedger sLedger;
std::map<std::string, std::string> sUnhandled;
uint64_t sAppliedRevision = 0;
bool sLedgerLoaded = false;

// Keyed per-game and per-save-tag: the other game's fresh-save reset cannot erase these records,
// and a save recreated under a new tag starts from an empty ledger and re-grants everything.
std::string LedgerPath() {
    const auto& state = OotmmSession_GetState();
    return state.GetBootConfig().StatePath + ".applied.oot-" + state.GetNativeSaveTag() + "." +
           std::to_string(gSaveContext.fileNum);
}

std::string LegacyLedgerPath() {
    return OotmmSession_GetState().GetBootConfig().StatePath + ".applied." +
           std::to_string(gSaveContext.fileNum);
}

void EnsureLedger() {
    if (sLedgerLoaded) {
        return;
    }
    // Older sessions shared one ledger between both games; adopt it once, then stay per-game.
    if (!sLedger.Load(LedgerPath())) {
        sLedger.Load(LegacyLedgerPath());
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
    // gItemSlots only covers inventory items, so SLOT()/INV_CONTENT() read past its end for equipment.
    Item_Give(play, item);
    SPDLOG_INFO("[OoTMM grant] gave {} (item {})", key, static_cast<int>(item));
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

std::vector<uint8_t> GrantedTradeItems(const std::vector<uint8_t>& candidates) {
    std::vector<uint8_t> granted;
    for (const auto& [itemId, count] : OotmmIpc_GetInventory().GetValues()) {
        for (const uint8_t item : candidates) {
            if (IsGrantedTradeItem(itemId, count, item)) {
                granted.push_back(item);
                break;
            }
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
    // Grants overwrite the slot, so remember natively obtained occupants before they vanish.
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
    return owned;
}

void ApplyOne(PlayState* play, const std::string& itemId, uint32_t count) {
    if (count == 0 || IsCustomItemBacked(itemId)) {
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
                const std::vector<uint8_t> tiers = LadderTiers(op.Slot);
                if (!tiers.empty()) {
                    const size_t tier = std::min<size_t>(op.Amount, tiers.size()) - 1;
                    const std::string key = itemId + ":" + std::to_string(tier);
                    if (!sLedger.Has(key)) {
                        ApplyOnce(play, key, tiers[tier]);
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
            case Ship::OotmmGrantKind::Health: {
                if (count > already) {
                    Health_ChangeBy(play, static_cast<int16_t>((count - already) * 16));
                    sLedger.SetApplied(itemId, count);
                }
                break;
            }
            case Ship::OotmmGrantKind::Magic: {
                if (count > already) {
                    Magic_Fill(play);
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
                const int scene = DungeonSceneOf(op.Slot);
                if (scene < 0 ||
                    scene >= static_cast<int>(ARRAY_COUNT(gSaveContext.inventory.dungeonItems))) {
                    break;
                }
                if (op.Slot.rfind("SMALL_KEY", 0) == 0) {
                    if (count > already &&
                        scene < static_cast<int>(ARRAY_COUNT(gSaveContext.inventory.dungeonKeys))) {
                        const s8 held = gSaveContext.inventory.dungeonKeys[scene];
                        gSaveContext.inventory.dungeonKeys[scene] =
                            static_cast<s8>((held < 0 ? 0 : held) + (count - already));
                        sLedger.SetApplied(itemId, count);
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
            case Ship::OotmmGrantKind::SmallKeyRing: {
                const int scene = DungeonSceneOf(op.Slot);
                if (already != 0 || scene < 0 ||
                    scene >= static_cast<int>(ARRAY_COUNT(gSaveContext.inventory.dungeonKeys))) {
                    break;
                }
                const int keys = OotmmDungeons_MaxSmallKeys(scene);
                if (keys == 0) {
                    break;
                }
                gSaveContext.inventory.dungeonKeys[scene] = static_cast<s8>(keys);
                sLedger.SetApplied(itemId, 1);
                break;
            }
            case Ship::OotmmGrantKind::Token: {
                if (count > already) {
                    gSaveContext.inventory.questItems |= gBitFlags[QUEST_SKULL_TOKEN];
                    gSaveContext.inventory.gsTokens += static_cast<s16>(count - already);
                    sLedger.SetApplied(itemId, count);
                }
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

} // namespace

// TODO: the Master Sword is randomized, so owning it does not mean the pedestal check was completed;
// this needs the check system to gate the final fight on actually having pulled it.
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
        // The debug-clear is asynchronous, so the removed item may still look granted.
        for (const uint8_t replacement : OwnedTradeItems(slot)) {
            if (replacement != item) {
                gSaveContext.inventory.items[slot] = replacement;
                break;
            }
        }
    }
}

void OotmmItemApply_ResetLedgerForNewSave() {
    if (!OotmmSession_IsActive()) {
        return;
    }
    sLedger.Reset();
    sLedger.Save(LedgerPath());
    sLedgerLoaded = true;
    sAppliedRevision = 0;
}

void OotmmItemApply_Reconcile() {
    if (gPlayState == nullptr || !OotmmSession_IsActive()) {
        return;
    }
    const auto& inventory = OotmmIpc_GetInventory();
    if (inventory.GetRevision() == sAppliedRevision) {
        return;
    }
    EnsureLedger();
    sAppliedRevision = inventory.GetRevision();
    size_t owned = 0;
    for (const auto& [itemId, count] : inventory.GetValues()) {
        if (count > 0) {
            owned++;
        }
        ApplyOne(gPlayState, itemId, count);
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

void OotmmItemApply_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        OotmmItemApply_Reconcile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        []() { OotmmTriforce_Update(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t) {
        sAppliedRevision = 0;
        sLedgerLoaded = false;
    });
}
