#include "OotmmNpcChecks.h"

#include "Enhancements/custom-message/CustomMessageManager.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "OotmmBottles.h"
#include "OotmmCheckLedger.h"
#include "OotmmChecks.h"
#include "OotmmDialog.h"
#include <libultraship/bridge/OotmmDialog.h>
#include "OotmmIpc.h"
#include <libultraship/bridge/OotmmBottles.h>
#include <libultraship/bridge/OotmmItemGrant.h>
#include <memory>
#include <vector>
#include "OotmmItemApply.h"
#include "OotmmItemModels.h"
#include "OotmmItemPresentation.h"
#include "OotmmSession.h"
#include "OotmmXflags.h"

#include <iterator>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "src/overlays/actors/ovl_En_Bom_Bowl_Pit/z_en_bom_bowl_pit.h"
#include "src/overlays/actors/ovl_En_Ex_Item/z_en_ex_item.h"
#include "src/overlays/actors/ovl_En_Dnt_Jiji/z_en_dnt_jiji.h"
#include "src/overlays/actors/ovl_En_Dns/z_en_dns.h"
#include "src/overlays/actors/ovl_En_Fr/z_en_fr.h"
#include "src/overlays/actors/ovl_Fishing/z_fishing.h"
#include "src/overlays/actors/ovl_En_Ge1/z_en_ge1.h"
#include "src/overlays/actors/ovl_En_Ta/z_en_ta.h"
#include "src/overlays/actors/ovl_En_Tk/z_en_tk.h"
#include "src/overlays/actors/ovl_En_Go2/z_en_go2.h"
#include "src/overlays/actors/ovl_En_Kz/z_en_kz.h"
#include "src/overlays/actors/ovl_En_Niw_Lady/z_en_niw_lady.h"
#include "src/overlays/actors/ovl_En_Ssh/z_en_ssh.h"
#include "src/overlays/actors/ovl_En_Sth/z_en_sth.h"

extern PlayState* gPlayState;
GetItemEntry ItemTable_Retrieve(int16_t getItemId);
}

namespace {

const Ship::OotmmPlacement* Find(const char* nativeId) {
    return nativeId != nullptr ? OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, "npc", nativeId) : nullptr;
}

bool IsCollected(const Ship::OotmmPlacement& placement) {
    return OotmmCheckLedger_Contains(placement.CheckId) || OotmmIpc_IsCheckCompleted(placement.CheckId);
}

constexpr const char* kSongSites[] = {
    "OOT_SHEIK_FOREST", "OOT_SHEIK_FIRE",       "OOT_SHEIK_WATER",       "OOT_SHEIK_SPIRIT",
    "OOT_SHEIK_SHADOW", "OOT_SHEIK_LIGHT",      "OOT_ZELDA_SONG",        "OOT_MALON_SONG",
    "OOT_SARIA_SONG",   "OOT_ROYAL_TOMB_SONG",  "OOT_OCARINA_TIME_SONG", "OOT_SONG_STORMS",
};

constexpr int16_t kVanillaCuccoCount = 7;

constexpr const char* kSkulltulaSites[] = {
    "OOT_GS_100", "OOT_GS_10", "OOT_GS_20", "OOT_GS_30", "OOT_GS_40", "OOT_GS_50",
};

const char* SkulltulaSite(int32_t params) {
    return (params >= 0 && params < static_cast<int32_t>(std::size(kSkulltulaSites)))
               ? kSkulltulaSites[params]
               : nullptr;
}

const char* BowlingSite(int32_t prizeIndex) {
    switch (prizeIndex) {
        case EXITEM_BOMB_BAG_BOWLING:
        case EXITEM_BOMB_BAG_COUNTER:
            return "OOT_BOMBCHU_BOWLING_1";
        case EXITEM_HEART_PIECE_BOWLING:
        case EXITEM_HEART_PIECE_COUNTER:
            return "OOT_BOMBCHU_BOWLING_2";
        default:
            return nullptr;
    }
}

const char* FishingSite() {
    return LINK_IS_ADULT ? "OOT_FISH_ADULT" : "OOT_FISH_CHILD";
}

bool IsFishingPrizeCatch(const VBFishingData* data) {
    return data != nullptr && data->fishWeight >= (LINK_IS_ADULT ? 50.0f : 44.0f);
}

int32_t Collect(const char* nativeId) {
    return nativeId != nullptr && OotmmNpcChecks_Collect(nativeId);
}

bool OwnsTradeItem(uint8_t slot, uint8_t item) {
    uint8_t items[32];
    const int32_t count = OotmmItemApply_OwnedTradeItems(slot, items, static_cast<int32_t>(std::size(items)));
    for (int32_t i = 0; i < count; i++) {
        if (items[i] == item) {
            return true;
        }
    }
    return false;
}

bool OwnsBombchuBag() {
    for (const auto& [id, count] : OotmmIpc_GetInventory().GetValues()) {
        if (count > 0 && id.find("BOMBCHU_BAG") != std::string::npos) {
            return true;
        }
    }
    return false;
}

const char* FrogSite(int32_t songIndex) {
    switch (songIndex) {
        case FROG_ZL:
            return "OOT_FROGS_ZL";
        case FROG_EPONA:
            return "OOT_FROGS_EPONA";
        case FROG_SARIA:
            return "OOT_FROGS_SARIA";
        case FROG_SUNS:
            return "OOT_FROGS_SUNS";
        case FROG_SOT:
            return "OOT_FROGS_SOT";
        case FROG_STORMS:
            return "OOT_FROGS_STORMS";
        case FROG_CHOIR_SONG:
            return "OOT_FROGS_GAME";
        default:
            return nullptr;
    }
}

const char* BlueWarpSite(int32_t item) {
    switch (item) {
        case ITEM_KOKIRI_EMERALD:
            return "OOT_BLUE_WARP_GOHMA";
        case ITEM_GORON_RUBY:
            return "OOT_BLUE_WARP_KING_DODONGO";
        case ITEM_ZORA_SAPPHIRE:
            return "OOT_BLUE_WARP_BARINADE";
        case ITEM_MEDALLION_FOREST:
            return "OOT_BLUE_WARP_PHANTOM_GANON";
        case ITEM_MEDALLION_FIRE:
            return "OOT_BLUE_WARP_VOLVAGIA";
        case ITEM_MEDALLION_WATER:
            return "OOT_BLUE_WARP_MORPHA";
        case ITEM_MEDALLION_SHADOW:
            return "OOT_BLUE_WARP_BONGO_BONGO";
        case ITEM_MEDALLION_SPIRIT:
            return "OOT_BLUE_WARP_TWINROVA";
        default:
            return nullptr;
    }
}

const char* BlueWarpSiteForScene(int32_t scene) {
    switch (scene) {
        case SCENE_DEKU_TREE_BOSS:
            return "OOT_BLUE_WARP_GOHMA";
        case SCENE_DODONGOS_CAVERN_BOSS:
            return "OOT_BLUE_WARP_KING_DODONGO";
        case SCENE_JABU_JABU_BOSS:
            return "OOT_BLUE_WARP_BARINADE";
        case SCENE_FOREST_TEMPLE_BOSS:
            return "OOT_BLUE_WARP_PHANTOM_GANON";
        case SCENE_FIRE_TEMPLE_BOSS:
            return "OOT_BLUE_WARP_VOLVAGIA";
        case SCENE_WATER_TEMPLE_BOSS:
            return "OOT_BLUE_WARP_MORPHA";
        case SCENE_SHADOW_TEMPLE_BOSS:
            return "OOT_BLUE_WARP_BONGO_BONGO";
        case SCENE_SPIRIT_TEMPLE_BOSS:
            return "OOT_BLUE_WARP_TWINROVA";
        default:
            return nullptr;
    }
}

const char* GoronSite(int32_t getItemId) {
    switch (getItemId) {
        case GI_PRESCRIPTION:
            return "OOT_TRADE_PRESCRIPTION";
        case GI_CLAIM_CHECK:
            return "OOT_TRADE_CLAIM_CHECK";
        case GI_SWORD_BGS:
            return "OOT_TRADE_BIGGORON_SWORD";
        case GI_TUNIC_GORON:
            return "OOT_GORON_LINK_TUNIC";
        case GI_BOMB_BAG_30:
        case GI_BOMB_BAG_40:
            return "OOT_GORON_BOMB_BAG";
        default:
            return nullptr;
    }
}

const char* DekuTheaterSite(int32_t getItemId) {
    switch (getItemId) {
        case GI_STICK_UPGRADE_20:
        case GI_STICK_UPGRADE_30:
            return "OOT_THEATER_STICKS";
        case GI_NUT_UPGRADE_30:
        case GI_NUT_UPGRADE_40:
            return "OOT_THEATER_NUTS";
        default:
            return nullptr;
    }
}

std::string PlacementName(const Ship::OotmmPlacement& placement) {
    const std::string& raw = placement.CloakItemId.empty() ? placement.ItemName : placement.CloakItemName;
    std::string name = "%g" + raw + "%w";
    for (const auto& [suffix, tag] : { std::pair<std::string_view, std::string_view>{ " (OoT)", "%cOoT" },
                                       { " (OOT)", "%cOoT" },
                                       { " (MM)", "%pMM" } }) {
        if (raw.ends_with(suffix)) {
            name = "%g" + raw.substr(0, raw.size() - suffix.size()) + "%w (" + std::string(tag) + "%w)";
            break;
        }
    }
    if (!placement.IsFor(OotmmSession_GetState().GetPlayerId())) {
        name += " for %yPlayer " + std::to_string(placement.OwnerPlayer) + "%w";
    }
    return name;
}

std::string RewardName(const char* nativeId) {
    const auto* placement = nativeId != nullptr ? Find(nativeId) : nullptr;
    return placement != nullptr ? PlacementName(*placement) : std::string();
}

constexpr uint32_t kScrubPriceBase = 0x40;
constexpr uint32_t kMerchantPriceBase = 0x66;

enum ScrubItem {
    kScrubNuts,
    kScrubStick,
    kScrubHeartPiece,
    kScrubSeeds,
    kScrubShieldDeku,
    kScrubBombs,
    kScrubSeedsArrows,
    kScrubPotionRed,
    kScrubPotionGreen,
    kScrubSticksUpgrade,
    kScrubNutsUpgrade,
};

constexpr int ScrubKey(int scene, bool grotto, int item) {
    return (grotto ? 0x1000000 : 0) | (scene << 16) | item;
}

int ScrubId(const Actor* scrub, const PlayState* play) {
    const bool grotto = play->sceneNum == SCENE_GROTTOS;
    int scene = play->sceneNum;
    if (grotto) {
        const int room = play->roomCtx.curRoom.num;
        scene = room == 0x06 ? SCENE_LOST_WOODS : room == 0x01 ? SCENE_HYRULE_FIELD : OotmmXflags_LastScene();
    }
    switch (ScrubKey(scene, grotto, scrub->params & 0xFF)) {
        case ScrubKey(SCENE_LOST_WOODS, false, kScrubSticksUpgrade): return 0x00;
        case ScrubKey(SCENE_LOST_WOODS, false, kScrubStick): return 0x01;
        case ScrubKey(SCENE_LOST_WOODS, false, kScrubNuts): return 0x02;
        case ScrubKey(SCENE_LOST_WOODS, true, kScrubNutsUpgrade): return 0x03;
        case ScrubKey(SCENE_LOST_WOODS, true, kScrubSeedsArrows): return 0x04;
        case ScrubKey(SCENE_SACRED_FOREST_MEADOW, true, kScrubPotionGreen): return 0x05;
        case ScrubKey(SCENE_SACRED_FOREST_MEADOW, true, kScrubPotionRed): return 0x06;
        case ScrubKey(SCENE_HYRULE_FIELD, true, kScrubHeartPiece): return 0x07;
        case ScrubKey(SCENE_LON_LON_RANCH, true, kScrubNuts): return 0x08;
        case ScrubKey(SCENE_LON_LON_RANCH, true, kScrubSeedsArrows): return 0x09;
        case ScrubKey(SCENE_LON_LON_RANCH, true, kScrubBombs): return 0x0a;
        case ScrubKey(SCENE_GORON_CITY, true, kScrubNuts): return 0x0b;
        case ScrubKey(SCENE_GORON_CITY, true, kScrubSeedsArrows): return 0x0c;
        case ScrubKey(SCENE_GORON_CITY, true, kScrubBombs): return 0x0d;
        case ScrubKey(SCENE_DEATH_MOUNTAIN_CRATER, false, kScrubBombs): return 0x0e;
        case ScrubKey(SCENE_DEATH_MOUNTAIN_CRATER, true, kScrubNuts): return 0x0f;
        case ScrubKey(SCENE_DEATH_MOUNTAIN_CRATER, true, kScrubSeedsArrows): return 0x10;
        case ScrubKey(SCENE_DEATH_MOUNTAIN_CRATER, true, kScrubBombs): return 0x11;
        case ScrubKey(SCENE_ZORAS_RIVER, true, kScrubPotionGreen): return 0x12;
        case ScrubKey(SCENE_ZORAS_RIVER, true, kScrubPotionRed): return 0x13;
        case ScrubKey(SCENE_LAKE_HYLIA, true, kScrubNuts): return 0x14;
        case ScrubKey(SCENE_LAKE_HYLIA, true, kScrubSeedsArrows): return 0x15;
        case ScrubKey(SCENE_LAKE_HYLIA, true, kScrubBombs): return 0x16;
        case ScrubKey(SCENE_GERUDO_VALLEY, true, kScrubPotionGreen): return 0x17;
        case ScrubKey(SCENE_GERUDO_VALLEY, true, kScrubPotionRed): return 0x18;
        case ScrubKey(SCENE_DESERT_COLOSSUS, true, kScrubPotionGreen): return 0x19;
        case ScrubKey(SCENE_DESERT_COLOSSUS, true, kScrubPotionRed): return 0x1a;
        case ScrubKey(SCENE_DEKU_TREE, false, kScrubShieldDeku): return 0x1b;
        case ScrubKey(SCENE_DODONGOS_CAVERN, false, kScrubSeeds): return 0x1c;
        case ScrubKey(SCENE_DODONGOS_CAVERN, false, kScrubStick): return 0x1d;
        case ScrubKey(SCENE_DODONGOS_CAVERN, false, kScrubNuts): return 0x1e;
        case ScrubKey(SCENE_DODONGOS_CAVERN, false, kScrubPotionRed): return 0x1e;
        case ScrubKey(SCENE_DODONGOS_CAVERN, false, kScrubShieldDeku): return 0x1f;
        case ScrubKey(SCENE_JABU_JABU, false, kScrubNuts): return 0x20;
        case ScrubKey(SCENE_INSIDE_GANONS_CASTLE, false, kScrubPotionGreen): return 0x21;
        case ScrubKey(SCENE_INSIDE_GANONS_CASTLE, false, kScrubBombs): return 0x22;
        case ScrubKey(SCENE_INSIDE_GANONS_CASTLE, false, kScrubSeedsArrows): return 0x23;
        case ScrubKey(SCENE_INSIDE_GANONS_CASTLE, false, kScrubPotionRed): return 0x24;
        case ScrubKey(SCENE_INSIDE_GANONS_CASTLE, false, kScrubNuts): return 0x25;
        default: return -1;
    }
}

const Ship::OotmmPlacement* FindScrub(const Actor* scrub, const PlayState* play, int* id = nullptr) {
    const int scrubId = ScrubId(scrub, play);
    if (id != nullptr) {
        *id = scrubId;
    }
    return scrubId < 0 ? nullptr
                       : OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, "scrub", std::to_string(scrubId));
}

bool IsShuffledScrub(const EnDns* scrub) {
    return scrub != nullptr && scrub->dnsItemEntry == &scrub->ootmmItemEntry;
}

constexpr OotmmRepeatItem kRepeatHeart{ GI_HEART, "OOT_RECOVERY_HEART", "Recovery Heart" };
std::string sPendingLedgerRepeat;
std::string sPendingLedgerName;

const std::unordered_map<std::string, OotmmRepeatItem> kRepeatItems = {
    { "OOT_RECOVERY_HEART", kRepeatHeart },
    { "OOT_STICK", { GI_STICKS_1, "OOT_STICK", "Deku Stick" } },
    { "OOT_STICKS_5", { GI_STICKS_5, "OOT_STICKS_5", "5 Deku Sticks" } },
    { "OOT_STICKS_10", { GI_STICKS_10, "OOT_STICKS_10", "10 Deku Sticks" } },
    { "OOT_NUTS_5", { GI_NUTS_5, "OOT_NUTS_5", "5 Deku Nuts" } },
    { "OOT_NUTS_5_ALT", { GI_NUTS_5_2, "OOT_NUTS_5", "5 Deku Nuts" } },
    { "OOT_NUTS_10", { GI_NUTS_10, "OOT_NUTS_10", "10 Deku Nuts" } },
    { "OOT_BOMB", { GI_BOMBS_1, "OOT_BOMB", "Bomb" } },
    { "OOT_BOMBS_5", { GI_BOMBS_5, "OOT_BOMBS_5", "5 Bombs" } },
    { "OOT_BOMBS_10", { GI_BOMBS_10, "OOT_BOMBS_10", "10 Bombs" } },
    { "OOT_BOMBS_20", { GI_BOMBS_20, "OOT_BOMBS_20", "20 Bombs" } },
    { "OOT_BOMBS_30", { GI_BOMBS_30, "OOT_BOMBS_30", "30 Bombs" } },
    { "OOT_ARROWS_5", { GI_ARROWS_SMALL, "OOT_ARROWS_5", "5 Arrows" } },
    { "OOT_ARROWS_10", { GI_ARROWS_MEDIUM, "OOT_ARROWS_10", "10 Arrows" } },
    { "OOT_ARROWS_30", { GI_ARROWS_LARGE, "OOT_ARROWS_30", "30 Arrows" } },
    { "OOT_DEKU_SEEDS_5", { GI_SEEDS_5, "OOT_DEKU_SEEDS_5", "5 Deku Seeds" } },
    { "OOT_DEKU_SEEDS_30", { GI_SEEDS_30, "OOT_DEKU_SEEDS_30", "30 Deku Seeds" } },
    { "OOT_BOMBCHU_5", { GI_BOMBCHUS_5, "OOT_BOMBCHU_5", "5 Bombchus" } },
    { "OOT_BOMBCHU_10", { GI_BOMBCHUS_10, "OOT_BOMBCHU_10", "10 Bombchus" } },
    { "OOT_BOMBCHU_20", { GI_BOMBCHUS_20, "OOT_BOMBCHU_20", "20 Bombchus" } },
    { "OOT_SHIELD_DEKU", { GI_SHIELD_DEKU, "OOT_SHIELD_DEKU", "Deku Shield" } },
    { "OOT_SHIELD_HYLIAN", { GI_SHIELD_HYLIAN, "OOT_SHIELD_HYLIAN", "Hylian Shield" } },
    { "OOT_MILK", { GI_MILK, "OOT_MILK", "Milk" } },
    { "OOT_BOTTLE_MILK", { GI_MILK, "OOT_MILK", "Milk" } },
    { "OOT_POTION_RED", { GI_POTION_RED, "OOT_POTION_RED", "Red Potion" } },
    { "OOT_BOTTLE_POTION_RED", { GI_POTION_RED, "OOT_POTION_RED", "Red Potion" } },
    { "OOT_POTION_GREEN", { GI_POTION_GREEN, "OOT_POTION_GREEN", "Green Potion" } },
    { "OOT_BOTTLE_POTION_GREEN", { GI_POTION_GREEN, "OOT_POTION_GREEN", "Green Potion" } },
    { "OOT_POTION_BLUE", { GI_POTION_BLUE, "OOT_POTION_BLUE", "Blue Potion" } },
    { "OOT_BOTTLE_POTION_BLUE", { GI_POTION_BLUE, "OOT_POTION_BLUE", "Blue Potion" } },
    { "OOT_FAIRY", { GI_FAIRY, "OOT_FAIRY", "Fairy" } },
    { "OOT_BOTTLE_FAIRY", { GI_FAIRY, "OOT_FAIRY", "Fairy" } },
    { "OOT_FISH", { GI_FISH, "OOT_FISH", "Fish" } },
    { "OOT_BUGS", { GI_BUGS, "OOT_BUGS", "Bugs" } },
    { "OOT_BLUE_FIRE", { GI_BLUE_FIRE, "OOT_BLUE_FIRE", "Blue Fire" } },
    { "OOT_BOTTLE_BLUE_FIRE", { GI_BLUE_FIRE, "OOT_BLUE_FIRE", "Blue Fire" } },
    { "OOT_POE", { GI_POE, "OOT_POE", "Poe" } },
    { "OOT_BOTTLE_POE", { GI_POE, "OOT_POE", "Poe" } },
    { "OOT_BOTTLE_BIG_POE", { GI_BIG_POE, "OOT_BIG_POE", "Big Poe" } },
    { "OOT_MAGIC_JAR_SMALL", { GI_MAGIC_SMALL, "OOT_MAGIC_JAR_SMALL", "Small Magic Jar" } },
    { "OOT_MAGIC_JAR_LARGE", { GI_MAGIC_LARGE, "OOT_MAGIC_JAR_LARGE", "Large Magic Jar" } },
};

bool IsLicensed(const std::string& itemId) {
    if (itemId != "OOT_BOMBCHU_5" && itemId != "OOT_BOMBCHU_10" && itemId != "OOT_BOMBCHU_20") {
        return true;
    }
    const auto behavior = OotmmSession_GetState().GetStringSetting("bombchuBehaviorOot", "bombBag");
    return (behavior != "bagFirst" && behavior != "bagSeparate") || INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU;
}

constexpr int32_t kUnlimitedRoom = 999;

static int32_t VitalFree(std::string_view family) {
    if (family == "health") {
        return gSaveContext.healthCapacity - gSaveContext.health;
    }
    if (family == "magic") {
        return gSaveContext.isMagicAcquired ? (gSaveContext.isDoubleMagicAcquired ? 0x60 : 0x30) - gSaveContext.magic : 0;
    }
    if (family == "fairy") {
        return VitalFree("health") > 0 || VitalFree("magic") > 0;
    }
    if (family == "sticks") {
        return CUR_UPG_VALUE(UPG_STICKS) == 0 ? kUnlimitedRoom : CUR_CAPACITY(UPG_STICKS) - AMMO(ITEM_STICK);
    }
    if (family == "nuts") {
        return CUR_UPG_VALUE(UPG_NUTS) == 0 ? kUnlimitedRoom : CUR_CAPACITY(UPG_NUTS) - AMMO(ITEM_NUT);
    }
    if (family == "bombs") {
        return CUR_UPG_VALUE(UPG_BOMB_BAG) > 0 ? CUR_CAPACITY(UPG_BOMB_BAG) - AMMO(ITEM_BOMB) : 0;
    }
    if (family == "bombchu") {
        return OotmmItemApply_BombchuCapacity() - AMMO(ITEM_BOMBCHU);
    }
    if (family == "arrows") {
        return CUR_UPG_VALUE(UPG_QUIVER) > 0 ? CUR_CAPACITY(UPG_QUIVER) - AMMO(ITEM_BOW) : 0;
    }
    if (family == "seeds") {
        return CUR_UPG_VALUE(UPG_BULLET_BAG) > 0 ? CUR_CAPACITY(UPG_BULLET_BAG) - AMMO(ITEM_SLINGSHOT) : 0;
    }
    if (family == "rupees") {
        return CUR_CAPACITY(UPG_WALLET) - gSaveContext.rupees;
    }
    if (family == "bottles") {
        const uint32_t emptyContent = OotmmBottles_ContentOf(ITEM_BOTTLE);
        int32_t empty = 0;
        for (int32_t bottle = 0; bottle < Ship::kOotmmBottleCount; bottle++) {
            empty += OotmmBottles_ReadSlot(bottle) == emptyContent;
        }
        return empty;
    }
    if (family == "SHIELD_DEKU") {
        return !CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_DEKU);
    }
    if (family == "SHIELD_HYLIAN") {
        return !CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_HYLIAN);
    }
    return 1;
}

static std::string MmVitalsPath() {
    return OotmmSession_GetState().GetBootConfig().StatePath + ".vitals.mm";
}

bool CanHoldItem(const std::string& itemId) {
    const bool sharedShields = OotmmSession_GetState().GetBoolSetting("sharedShields", false);
    const std::string family = Ship::OotmmItemGrant::VitalFamily(itemId);
    if (itemId.starts_with("MM_")) {
        if (sharedShields && ((family == "SHIELD_DEKU" && VitalFree("SHIELD_DEKU") > 0) ||
                              (family == "SHIELD_HERO" && VitalFree("SHIELD_HYLIAN") > 0))) {
            return true;
        }
        return Ship::OotmmItemGrant::CanHoldRemote(itemId, OotmmIpc_GetInventory(), MmVitalsPath());
    }
    if (VitalFree(family) > 0) {
        return true;
    }
    if (sharedShields && (family == "SHIELD_DEKU" || family == "SHIELD_HYLIAN")) {
        return Ship::OotmmItemGrant::CanHoldRemote(family == "SHIELD_DEKU" ? "MM_SHIELD_DEKU" : "MM_SHIELD_HERO",
                                                   OotmmIpc_GetInventory(), MmVitalsPath());
    }
    return false;
}

const Ship::OotmmPlacement* RutoSapphirePlacement() {
    static constexpr const char* kRewardChecks[] = {
        "OOT Deku Tree Boss",      "OOT Dodongo Cavern Boss", "OOT Jabu-Jabu Boss",
        "OOT Forest Temple Boss",  "OOT Fire Temple Boss",    "OOT Water Temple Boss",
        "OOT Shadow Temple Boss",  "OOT Spirit Temple Boss",  "MM Woodfall Temple Boss",
        "MM Snowhead Temple Boss", "MM Great Bay Temple Boss", "MM Stone Tower Temple Inverted Boss",
    };
    static const Ship::OotmmPlacement* sPlacement = nullptr;
    static bool sResolved = false;
    if (sResolved) {
        return sPlacement;
    }
    sResolved = true;
    const auto& state = OotmmSession_GetState();
    for (uint32_t boss = 0; boss < std::size(kRewardChecks); boss++) {
        if (state.GetBossDungeon(boss) != 2u) {
            continue;
        }
        for (const auto& placement : state.GetPlacements()) {
            if (placement.CheckName == kRewardChecks[boss]) {
                sPlacement = &placement;
                break;
            }
        }
        break;
    }
    return sPlacement;
}

void BuildRutoSapphireText(uint16_t* textId, bool* loadFromMessageTable) {
    const auto* placement = OotmmSession_IsActive() ? RutoSapphirePlacement() : nullptr;
    if (placement == nullptr) {
        return;
    }
    CustomMessage message("Princess Ruto got " + PlacementName(*placement) + "!\nBut why Princess Ruto?");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BuildScrubText(uint16_t* textId, bool* loadFromMessageTable) {
    if (gPlayState == nullptr) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr || player->talkActor == nullptr || player->talkActor->id != ACTOR_EN_DNS ||
        *textId != player->talkActor->textId) {
        return;
    }
    const auto* scrub = reinterpret_cast<const EnDns*>(player->talkActor);
    const auto* placement = IsShuffledScrub(scrub) ? FindScrub(&scrub->actor, gPlayState) : nullptr;
    if (placement == nullptr) {
        return;
    }
    const std::string name = scrub->ootmmItemEntry.getItemId != GI_NONE
                                 ? "%g" + std::string(OotmmNpcChecks_FindRepeat(*placement)->Name) + "%w"
                                 : PlacementName(*placement);
    OotmmDialog::Load("Do you want " + name + " for %r" + Ship::OotmmDialogPrice(scrub->dnsItemEntry->itemPrice) + "%w?",
                      { "Yes", "No" });
    *loadFromMessageTable = false;
}

void LoadMerchantOffer(const char* site, int merchant, int vanillaPrice, bool* loadFromMessageTable) {
    const auto* placement = Find(site);
    if (placement == nullptr) {
        return;
    }
    const std::string price = Ship::OotmmDialogPrice(OotmmNpcChecks_MerchantPrice(merchant, vanillaPrice));
    const auto* repeat = OotmmNpcChecks_FindRepeat(*placement);
    const std::string name = !IsCollected(*placement)
                                 ? PlacementName(*placement)
                                 : "%g" + std::string(repeat != nullptr ? repeat->Name : kRepeatHeart.Name) + "%w";
    if (merchant == OOTMM_MERCHANT_MEDIGORON) {
        OotmmDialog::Load("How about buying " + name + " for %r" + price + "%w?", { "Buy", "Don't buy" });
    } else {
        OotmmDialog::Load(name + "!\nHow about %r" + price + "%w?", { "Yes", "No" });
    }
    *loadFromMessageTable = false;
}

void LoadCantBuyText(bool* loadFromMessageTable) {
    CustomMessage message("You can't buy that right now!");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BuildCarpetCantBuyText(uint16_t*, bool* loadFromMessageTable) {
    if (Find("OOT_CARPET_MERCHANT") != nullptr) {
        LoadCantBuyText(loadFromMessageTable);
    }
}

void BuildTalonCantBuyText(uint16_t*, bool* loadFromMessageTable) {
    if (Find("OOT_TALON_MILK") != nullptr) {
        LoadCantBuyText(loadFromMessageTable);
    }
}

void BuildGrannyCantBuyText(uint16_t*, bool* loadFromMessageTable) {
    Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
    if (player != nullptr && player->talkActor != nullptr && player->talkActor->id == ACTOR_EN_DS &&
        Find("OOT_WITCH_BLUE_POTION") != nullptr) {
        LoadCantBuyText(loadFromMessageTable);
    }
}

void BuildMedigoronOfferText(uint16_t*, bool* loadFromMessageTable) {
    if (!OotmmNpcChecks_IsCollected("OOT_MEDIGORON")) {
        LoadMerchantOffer("OOT_MEDIGORON", OOTMM_MERCHANT_MEDIGORON, 200, loadFromMessageTable);
    }
}

void BuildCarpetMerchantText(uint16_t*, bool* loadFromMessageTable) {
    LoadMerchantOffer("OOT_CARPET_MERCHANT", OOTMM_MERCHANT_CARPET, 200, loadFromMessageTable);
}

void BuildGrannyOfferText(uint16_t*, bool* loadFromMessageTable) {
    LoadMerchantOffer("OOT_WITCH_BLUE_POTION", OOTMM_MERCHANT_GRANNY, 100, loadFromMessageTable);
}

void BuildTalonMilkText(uint16_t*, bool* loadFromMessageTable) {
    LoadMerchantOffer("OOT_TALON_MILK", OOTMM_MERCHANT_TALON, 30, loadFromMessageTable);
}

void BuildSkulltulaHouseText(uint16_t*, bool* loadFromMessageTable) {
    if (gPlayState == nullptr) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr || player->talkActor == nullptr || player->talkActor->id != ACTOR_EN_SSH) {
        return;
    }
    const int32_t params = player->talkActor->params;
    const char* site = SkulltulaSite(params);
    if (site == nullptr || (params == ENSSH_FATHER && Find(site) == nullptr)) {
        return;
    }
    const auto& hinted = OotmmSession_GetState().GetHints().StaticReward(
        static_cast<Ship::OotmmStaticReward>(static_cast<size_t>(Ship::OotmmStaticReward::Gs100) + params));
    const std::string reward = hinted.empty() ? RewardName(site) : hinted;
    if (reward.empty()) {
        return;
    }
    const int32_t tokens = params == 0 ? 100 : params * 10;
    CustomMessage message("Yeaaarrgh! I'm cursed!!&Please save me by destroying %r" + std::to_string(tokens) +
                          " Spiders of the Curse%w and I will give you " + reward + ".");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

} // namespace

std::string OotmmNpcChecks_PlacementName(const Ship::OotmmPlacement& placement) {
    return PlacementName(placement);
}

const OotmmRepeatItem* OotmmNpcChecks_FindRepeat(const Ship::OotmmPlacement& placement) {
    if (!placement.CloakItemId.empty() || !placement.IsFor(OotmmSession_GetState().GetPlayerId())) {
        return nullptr;
    }
    const auto found = kRepeatItems.find(placement.ItemId);
    if (found != kRepeatItems.end()) {
        return &found->second;
    }
    static std::unordered_map<std::string, std::pair<std::string, OotmmRepeatItem>> sLedgerRepeats;
    const auto cached = sLedgerRepeats.find(placement.ItemId);
    if (cached != sLedgerRepeats.end()) {
        return &cached->second.second;
    }
    std::string renewal = Ship::OotmmItemGrant::RenewalOf(placement.ItemId);
    if (renewal.empty()) {
        return nullptr;
    }
    static std::vector<std::unique_ptr<std::string>> sStrings;
    sStrings.push_back(std::make_unique<std::string>(std::move(renewal)));
    const char* id = sStrings.back()->c_str();
    sStrings.push_back(std::make_unique<std::string>(Ship::OotmmItemCatalog::TrimGameSuffix(placement.ItemName)));
    const char* name = sStrings.back()->c_str();
    auto& entry = sLedgerRepeats[placement.ItemId];
    entry.second = OotmmRepeatItem{ GI_HEART, id, name, true };
    return &entry.second;
}

static void PresentLedgerRepeat(const std::string& itemId, const std::string& name) {
    OotmmIpc_SendRenewalGrant(itemId);
    Ship::GameIpcItemPresentation presentation;
    presentation.ItemId = itemId;
    presentation.ItemName = name;
    presentation.ItemGame = itemId.starts_with("MM_") ? "mm" : "oot";
    presentation.DestinationGame = presentation.ItemGame;
    OotmmItemPresentation_Queue(presentation);
}

static bool ConsumeLedgerRepeat() {
    if (sPendingLedgerRepeat.empty()) {
        return false;
    }
    PresentLedgerRepeat(sPendingLedgerRepeat, sPendingLedgerName);
    sPendingLedgerRepeat.clear();
    return true;
}

static void WriteVitals() {
    static int sFrames = 0;
    static std::string sLast;
    if (gPlayState == nullptr || !OotmmSession_IsActive() || ++sFrames < 20) {
        return;
    }
    sFrames = 0;
    std::string body = Ship::OotmmItemGrant::BuildVitals(Ship::OotmmGrantGame::Oot, VitalFree);
    if (body != sLast) {
        Ship::OotmmItemGrant::WriteVitals(OotmmSession_GetState().GetBootConfig().StatePath + ".vitals.oot", body);
        sLast = std::move(body);
    }
}

void OotmmNpcChecks_GiveRepeat(const OotmmRepeatItem& repeat, bool present) {
    sPendingLedgerRepeat.clear();
    if (repeat.ViaLedger) {
        if (present) {
            PresentLedgerRepeat(repeat.ItemId, repeat.Name);
        } else {
            OotmmIpc_SendRenewalGrant(repeat.ItemId);
        }
    } else {
        Item_Give(gPlayState, OotmmNpcChecks_RepeatItem(repeat));
    }
}

int OotmmNpcChecks_RepeatItem(const OotmmRepeatItem& repeat) {
    return ItemTable_Retrieve(repeat.Gi).itemId;
}

int OotmmNpcChecks_SaleResult(const Ship::OotmmPlacement& placement, bool heartFallback, int price) {
    const bool collected = IsCollected(placement);
    const auto* repeat = collected ? OotmmNpcChecks_FindRepeat(placement) : nullptr;
    if (collected && repeat == nullptr && heartFallback) {
        repeat = &kRepeatHeart;
    }
    if ((collected && repeat == nullptr) || (!collected && Ship::OotmmItemCatalog::IsNothing(placement.ItemId))) {
        return 1;
    }
    const auto& state = OotmmSession_GetState();
    const std::string itemId = repeat != nullptr ? repeat->ItemId : placement.ItemId;
    if (repeat != nullptr || placement.IsFor(state.GetPlayerId())) {
        if (!IsLicensed(itemId) || (state.GetStringSetting("mode", "single") == "single" && !CanHoldItem(itemId))) {
            return 1;
        }
    }
    if (gSaveContext.rupees < price) {
        return 0;
    }
    sPendingLedgerRepeat = repeat != nullptr && repeat->ViaLedger ? repeat->ItemId : "";
    sPendingLedgerName = repeat != nullptr && repeat->ViaLedger ? repeat->Name : "";
    return 2;
}

extern "C" int OotmmNpcChecks_MerchantPrice(int merchant, int vanillaPrice) {
    if (!OotmmSession_IsActive()) {
        return vanillaPrice;
    }
    return OotmmSession_GetState().GetPrice(kMerchantPriceBase + merchant).value_or(vanillaPrice);
}

extern "C" int OotmmNpcChecks_ScrubPrice(Actor* scrub, PlayState* play, int vanillaPrice) {
    int id = -1;
    const auto* placement = scrub != nullptr && play != nullptr ? FindScrub(scrub, play, &id) : nullptr;
    if (placement == nullptr) {
        return -1;
    }
    return OotmmSession_GetState().GetPrice(kScrubPriceBase + id).value_or(vanillaPrice);
}

extern "C" int OotmmNpcChecks_ScrubRepeatGi(Actor* scrub) {
    const auto* placement = scrub != nullptr && gPlayState != nullptr ? FindScrub(scrub, gPlayState) : nullptr;
    if (placement == nullptr || !IsCollected(*placement)) {
        return GI_NONE;
    }
    const auto* repeat = OotmmNpcChecks_FindRepeat(*placement);
    return repeat != nullptr && !repeat->ViaLedger ? repeat->Gi : GI_NONE;
}

extern "C" int OotmmNpcChecks_ScrubCanBuy(Actor* scrub, int price) {
    const auto* placement = scrub != nullptr && gPlayState != nullptr ? FindScrub(scrub, gPlayState) : nullptr;
    return placement != nullptr ? OotmmNpcChecks_SaleResult(*placement, false, price) : 1;
}

extern "C" int OotmmNpcChecks_MerchantRepeatGi(const char* nativeId, int vanillaGi) {
    const auto* placement = Find(nativeId);
    if (placement == nullptr) {
        return vanillaGi;
    }
    if (!IsCollected(*placement)) {
        return GI_NONE;
    }
    const auto* repeat = OotmmNpcChecks_FindRepeat(*placement);
    if (repeat != nullptr && repeat->ViaLedger) {
        return GI_NONE;
    }
    return repeat != nullptr ? repeat->Gi : kRepeatHeart.Gi;
}

extern "C" int OotmmNpcChecks_MerchantCanBuy(const char* nativeId, int price) {
    const auto* placement = Find(nativeId);
    return placement != nullptr ? OotmmNpcChecks_SaleResult(*placement, true, price) : -1;
}

extern "C" int OotmmNpcChecks_DrawRutoSapphire(Actor* actor, PlayState* play) {
    const auto* placement = OotmmSession_IsActive() ? RutoSapphirePlacement() : nullptr;
    if (placement == nullptr) {
        return 0;
    }
    const bool cloaked = !placement->CloakItemId.empty();
    float angle = (play->state.frames % 100) * (1.0f / 100.0f) * 6.2831855f;
    if (cloaked) {
        angle = -angle;
    }
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, MTXMODE_NEW);
    Matrix_Scale(0.35f, 0.35f, 0.35f, MTXMODE_APPLY);
    Matrix_RotateY(angle, MTXMODE_APPLY);
    return OotmmItemModel_DrawById(play, cloaked ? placement->CloakItemId : placement->ItemId,
                                   cloaked ? placement->CloakItemName : placement->ItemName)
               ? 1
               : 0;
}

extern "C" int OotmmNpcChecks_IsShuffled(const char* nativeId) {
    return Find(nativeId) != nullptr;
}

extern "C" const char* OotmmNpcChecks_BowlingPrizeSite(int prizeIndex) {
    return BowlingSite(prizeIndex);
}

extern "C" const char* OotmmNpcChecks_DekuTheaterPrizeSite(int getItemId) {
    return DekuTheaterSite(getItemId);
}

extern "C" int OotmmNpcChecks_DrawCheckItem(PlayState* play, const char* nativeId) {
    const auto* placement = Find(nativeId);
    if (play == nullptr || placement == nullptr) {
        return 0;
    }
    const std::string& itemId = placement->CloakItemId.empty() ? placement->ItemId : placement->CloakItemId;
    const std::string& itemName = placement->CloakItemId.empty() ? placement->ItemName : placement->CloakItemName;
    return OotmmItemModel_DrawById(play, itemId, itemName) ? 1 : 0;
}

extern "C" int OotmmNpcChecks_BlueWarpReady(PlayState* play) {
    if (play == nullptr) {
        return 1;
    }
    const char* site = BlueWarpSiteForScene(play->sceneNum);
    const auto* placement = Find(site);
    if (placement == nullptr) {
        return 1;
    }
    if (!IsCollected(*placement)) {
        OotmmNpcChecks_Collect(site);
        return 0;
    }
    Player* player = GET_PLAYER(play);
    return player != nullptr && !OotmmIpc_IsCheckPresentationPending(placement->CheckId) &&
           Message_GetState(&play->msgCtx) == TEXT_STATE_NONE &&
           !(player->stateFlags1 & (PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_TALKING | PLAYER_STATE1_IN_CUTSCENE));
}

extern "C" void OotmmNpcChecks_DrawBlueWarpReward(Actor* actor, PlayState* play) {
    if (actor == nullptr || play == nullptr) {
        return;
    }
    const char* site = BlueWarpSiteForScene(play->sceneNum);
    const auto* placement = Find(site);
    if (placement == nullptr || IsCollected(*placement)) {
        return;
    }
    Matrix_Push();
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y + 35.0f, actor->world.pos.z, MTXMODE_NEW);
    Matrix_Scale(0.35f, 0.35f, 0.35f, MTXMODE_APPLY);
    Matrix_RotateY((play->state.frames % 100) * (6.28318530717958647692f / 100.0f), MTXMODE_APPLY);
    if (!OotmmNpcChecks_DrawCheckItem(play, site)) {
        GetItem_Draw(play, GID_RUPEE_GREEN);
    }
    Matrix_Pop();
}

extern "C" int OotmmNpcChecks_IsCollected(const char* nativeId) {
    const auto* placement = Find(nativeId);
    return placement != nullptr && IsCollected(*placement);
}

extern "C" int OotmmNpcChecks_Collect(const char* nativeId) {
    const auto* placement = Find(nativeId);
    if (placement == nullptr) {
        return 0;
    }
    OotmmIpc_SendCheckCollected(placement->CheckId, !IsCollected(*placement));
    OotmmCheckLedger_Record(placement->CheckId);
    return 1;
}

extern "C" int OotmmNpcChecks_TryCollectFreestanding(Actor* actor, PlayState* play, const char* nativeId,
                                                   float xzRange, float yRange) {
    const auto* placement = Find(nativeId);
    if (actor == nullptr || play == nullptr || placement == nullptr) {
        return 0;
    }
    if (IsCollected(*placement)) {
        return 1;
    }
    Player* player = GET_PLAYER(play);
    if (player == nullptr || actor->xzDistToPlayer >= xzRange || std::fabs(actor->yDistToPlayer) >= yRange ||
        play->msgCtx.msgMode != MSGMODE_NONE || play->pauseCtx.state != 0 ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || OotmmItemPresentation_IsPending() ||
        Player_GetExplosiveHeld(player) >= 0 || OotmmItemPresentation_PlayerReceiving(player) ||
        (player->stateFlags1 &
         (PLAYER_STATE1_DEAD | PLAYER_STATE1_CHARGING_SPIN_ATTACK | PLAYER_STATE1_HANGING_OFF_LEDGE |
          PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_JUMPING | PLAYER_STATE1_FREEFALL |
          PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_CARRYING_ACTOR |
          PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_TALKING))) {
        return 0;
    }
    return OotmmNpcChecks_Collect(nativeId);
}

extern "C" int OotmmNpcChecks_Done(const char* nativeId, int vanillaDone) {
    return OotmmNpcChecks_IsShuffled(nativeId) ? OotmmNpcChecks_IsCollected(nativeId) : vanillaDone;
}

extern "C" int OotmmNpcChecks_DungeonBeaten(int blueWarpFlag, int vanillaOwned) {
    return OotmmSession_IsActive() ? Flags_GetEventChkInf(blueWarpFlag) != 0 : vanillaOwned;
}

extern "C" void OotmmNpcChecks_HatchEgg(PlayState* play, int adult) {
    const uint8_t slotItem = adult ? ITEM_TRADE_ADULT : ITEM_TRADE_CHILD;
    const uint8_t egg = adult ? ITEM_POCKET_EGG : ITEM_WEIRD_EGG;
    if (play == nullptr || INV_CONTENT(slotItem) != egg) {
        return;
    }
    const uint8_t slot = adult ? SLOT_TRADE_ADULT : SLOT_TRADE_CHILD;
    if (!Collect(adult ? "OOT_POCKET_EGG" : "OOT_WEIRD_EGG")) {
        const uint8_t hatched = adult ? ITEM_POCKET_CUCCO : ITEM_CHICKEN;
        Inventory_ReplaceItem(play, egg, hatched);
        OotmmItemApply_SetTradeItemOwned(slot, hatched, 1);
    }
    OotmmItemApply_ConsumeTradeItem(slot, egg);
}

static void ConsumeAdultTrade(uint8_t item) {
    OotmmItemApply_ConsumeTradeItem(SLOT_TRADE_ADULT, item);
}

void OotmmNpcChecks_Init(void) {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(WriteVitals);
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_SONG, {
        const int item = va_arg(args, int);
        if (item >= ITEM_SONG_MINUET && item <= ITEM_SONG_STORMS &&
            OotmmNpcChecks_Collect(kSongSites[item - ITEM_SONG_MINUET])) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_SHIEK_PREPARE_TO_GIVE_SERENADE_OF_WATER, {
        *should = !OotmmNpcChecks_IsCollected("OOT_SHEIK_WATER");
    });

    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_SERENADE_OF_WATER, {
        Player* player = gPlayState != nullptr ? GET_PLAYER(gPlayState) : nullptr;
        *should = player != nullptr && !OotmmNpcChecks_IsCollected("OOT_SHEIK_WATER") &&
                  Flags_GetTreasure(gPlayState, 0x02) && !(player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM);
    });

    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_PRELUDE_OF_LIGHT, {
        *should = CHECK_QUEST_ITEM(QUEST_MEDALLION_FOREST) && !OotmmNpcChecks_IsCollected("OOT_SHEIK_LIGHT");
    });

    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_SARIAS_SONG, {
        if (OotmmNpcChecks_IsShuffled("OOT_SARIA_SONG")) {
            *should = !OotmmNpcChecks_IsCollected("OOT_SARIA_SONG");
        }
    });
    REGISTER_VB_SHOULD(VB_MALON_RETURN_FROM_CASTLE, {
        if (OotmmSession_IsActive()) {
            *should = OotmmNpcChecks_Done("OOT_MALON_EGG",
                                          Flags_GetEventChkInf(EVENTCHKINF_OBTAINED_POCKET_EGG));
        }
    });

    REGISTER_VB_SHOULD(VB_MALON_ALREADY_TAUGHT_EPONAS_SONG, {
        if (OotmmNpcChecks_IsShuffled("OOT_MALON_SONG")) {
            *should = OotmmNpcChecks_IsCollected("OOT_MALON_SONG");
        }
    });
    REGISTER_VB_SHOULD(VB_PLAY_SUNS_SONG_CS, {
        if (OotmmNpcChecks_IsShuffled("OOT_ROYAL_TOMB_SONG")) {
            *should = !OotmmNpcChecks_IsCollected("OOT_ROYAL_TOMB_SONG");
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_SKULLTULA_REWARD, {
        EnSth* sth = va_arg(args, EnSth*);
        if (sth != nullptr && Collect(SkulltulaSite(sth->actor.params))) {
            *should = false;
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(BuildSkulltulaHouseText);

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(BuildScrubText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x4050, BuildRutoSapphireText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x304F, BuildMedigoronOfferText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x6077, BuildCarpetMerchantText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x500C, BuildGrannyOfferText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x208B, BuildTalonMilkText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x6073, BuildCarpetCantBuyText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x208A, BuildTalonCantBuyText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x96, BuildGrannyCantBuyText);
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_BUSINESS_SCRUB, {
        EnDns* scrub = va_arg(args, EnDns*);
        if (!IsShuffledScrub(scrub)) {
            return;
        }
        if (scrub->ootmmItemEntry.getItemId != GI_NONE) {
            return;
        }
        const auto* placement = FindScrub(&scrub->actor, gPlayState);
        if (placement != nullptr && !IsCollected(*placement)) {
            OotmmIpc_SendCheckCollected(placement->CheckId, true);
            OotmmCheckLedger_Record(placement->CheckId);
        } else {
            ConsumeLedgerRepeat();
        }
        *should = false;
    });
    REGISTER_VB_SHOULD(VB_BUSINESS_SCRUB_DESPAWN, {
        Actor* scrub = va_arg(args, Actor*);
        const auto* placement = scrub != nullptr ? FindScrub(scrub, gPlayState) : nullptr;
        if (placement != nullptr) {
            *should = IsCollected(*placement) && OotmmNpcChecks_FindRepeat(*placement) == nullptr;
        }
    });
    REGISTER_VB_SHOULD(VB_NEED_BOTTLE_FOR_GRANNYS_ITEM, {
        const int result = OotmmNpcChecks_MerchantCanBuy("OOT_WITCH_BLUE_POTION", 0);
        if (result >= 0) {
            *should = result == 1;
        }
    });

    REGISTER_VB_SHOULD(VB_SET_CUCCO_COUNT, {
        EnNiwLady* lady = va_arg(args, EnNiwLady*);
        if (lady != nullptr) {
            lady->cuccosInPen = kVanillaCuccoCount - 1;
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_ANJU_AS_CHILD, {
        if (Collect("OOT_ANJU_BOTTLE")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_ANJU_AS_ADULT, {
        if (Collect("OOT_TRADE_POCKET_EGG")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_TRADE_POCKET_CUCCO, {
        ConsumeAdultTrade(ITEM_POCKET_CUCCO);
        if (Collect("OOT_TRADE_COJIRO")) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_TRADE_COJIRO, {
        ConsumeAdultTrade(ITEM_COJIRO);
        if (Collect("OOT_TRADE_ODD_MUSHROOM")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_TRADE_ODD_MUSHROOM, {
        ConsumeAdultTrade(ITEM_ODD_MUSHROOM);
        if (Collect("OOT_TRADE_ODD_POTION")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_TRADE_ODD_POTION, {
        ConsumeAdultTrade(ITEM_ODD_POTION);
        if (Collect("OOT_TRADE_POACHER_SAW")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_TRADE_SAW, {
        ConsumeAdultTrade(ITEM_SAW);
        if (Collect("OOT_TRADE_BROKEN_GORON_SWORD")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_TRADE_FROG, {
        ConsumeAdultTrade(ITEM_FROG);
        if (Collect("OOT_TRADE_EYE_DROPS")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_LAB_DIVE, {
        if (Collect("OOT_LABORATORY_DIVE")) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_ADULT_KING_ZORA_ITEM_GIVE, {
        EnKz* kz = va_arg(args, EnKz*);
        if (kz != nullptr && kz->isTrading) {
            ConsumeAdultTrade(ITEM_PRESCRIPTION);
        }
        if (kz != nullptr &&
            Collect(kz->isTrading ? "OOT_TRADE_EYEBALL_FROG" : "OOT_ZORA_KING_TUNIC")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_GORON, {
        EnGo2* goron = va_arg(args, EnGo2*);
        if (goron != nullptr && goron->getItemId == GI_PRESCRIPTION) {
            ConsumeAdultTrade(ITEM_SWORD_BROKEN);
        } else if (goron != nullptr && goron->getItemId == GI_CLAIM_CHECK) {
            ConsumeAdultTrade(ITEM_EYEDROPS);
        }
        if (goron != nullptr && Collect(GoronSite(goron->getItemId))) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_USE_EYEDROP_DIALOGUE, { *should = false; });
    REGISTER_VB_SHOULD(VB_TRADE_TIMER_ODD_MUSHROOM, { *should = false; });
    REGISTER_VB_SHOULD(VB_TRADE_TIMER_EYEDROPS, { *should = false; });
    REGISTER_VB_SHOULD(VB_TRADE_TIMER_FROG, { *should = false; });
    REGISTER_VB_SHOULD(VB_BIGGORON_CONSIDER_SWORD_FORGED, { *should = true; });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_BLUE_WARP, {
        const int item = va_arg(args, int);
        if (Collect(BlueWarpSite(item))) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_PLAY_BLUE_WARP_CS, {
        if (gPlayState != nullptr) {
            const char* site = nullptr;
            if (gPlayState->sceneNum == SCENE_SPIRIT_TEMPLE_BOSS) {
                site = "OOT_BLUE_WARP_TWINROVA";
            } else if (gPlayState->sceneNum == SCENE_SHADOW_TEMPLE_BOSS) {
                site = "OOT_BLUE_WARP_BONGO_BONGO";
            }
            if (site != nullptr && OotmmNpcChecks_IsShuffled(site)) {
                *should = !OotmmNpcChecks_IsCollected(site);
            }
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_MASTER_SWORD, {
        if (Collect("OOT_MASTER_SWORD")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_LIGHT_MEDALLION, {
        if (Collect("OOT_LIGHT_MEDALLION")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_OCARINA_OF_TIME, {
        if (Find("OOT_OCARINA_TIME_ITEM") != nullptr) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FAIRY_OCARINA, {
        if (Collect("OOT_SARIA_OCARINA")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_ZELDAS_LETTER, {
        if (Collect("OOT_ZELDA_LETTER")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_LIGHT_ARROW, {
        if (Collect("OOT_ZELDA_LIGHT_ARROW")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_GERUDO_MEMBERSHIP_CARD, {
        if (Collect("OOT_GERUDO_CARD")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_MAN_ON_ROOF, {
        if (Collect("OOT_KAKARIKO_ROOF_MAN")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_WEIRD_EGG, {
        if (Collect("OOT_MALON_EGG")) {
            Flags_SetEventChkInf(EVENTCHKINF_SPOKE_TO_CHILD_MALON_AT_CASTLE_OR_MARKET);
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_SHOOTING_GALLERY, {
        if (Collect(LINK_IS_ADULT ? "OOT_SHOOTING_GAME_ADULT" : "OOT_SHOOTING_GAME_CHILD")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_ADULT_SHOOTING_GAME_REWARD, {
        if (OotmmNpcChecks_IsShuffled("OOT_SHOOTING_GAME_ADULT")) {
            *should = true;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_DIVING_MINIGAME, {
        if (Collect("OOT_ZORA_DIVING_GAME")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_TARGET_IN_WOODS, {
        if (Collect("OOT_LOST_WOODS_TARGET")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_SKULL_KID_SARIAS_SONG, {
        if (Collect("OOT_LOST_WOODS_SKULL_KID")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_TALONS_CHICKENS, {
        EnTa* talon = reinterpret_cast<EnTa*>(va_arg(args, Actor*));
        if (talon != nullptr && talon->ootmmRepeatGi != GI_NONE) {
            return;
        }
        if (ConsumeLedgerRepeat()) {
            *should = false;
            return;
        }
        if (talon != nullptr && Collect((talon->unk_2E0 & 2) ? "OOT_TALON_MILK" : "OOT_TALON_BOTTLE")) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_FROGS_GO_TO_IDLE, {
        EnFr* frog = va_arg(args, EnFr*);
        if (frog != nullptr) {
            const char* site = FrogSite(frog->songIndex);
            if (site != nullptr && !OotmmNpcChecks_IsCollected(site) && Collect(site)) {
                *should = true;
            }
        }
    });

    REGISTER_VB_SHOULD(VB_DAMPE_IN_GRAVEYARD_DESPAWN, {
        if (gPlayState != nullptr && !Flags_GetCollectible(gPlayState, 0x1F)) {
            Flags_UnsetItemGetInf(ITEMGETINF_1C);
        }
    });
    REGISTER_VB_SHOULD(VB_BE_VALID_GRAVEDIGGING_SPOT, {
        EnTk* dampe = va_arg(args, EnTk*);
        if (dampe != nullptr) {
            dampe->validDigHere = 1;
            *should = true;
        }
    });
    REGISTER_VB_SHOULD(VB_BE_DAMPE_GRAVEDIGGING_GRAND_PRIZE, {
        if (!Flags_GetItemGetInf(ITEMGETINF_1C)) {
            *should = true;
        }
    });
    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_CHILD_ROLLING_GORON_REWARD, { *should = true; });
    REGISTER_VB_SHOULD(VB_GORONS_CONSIDER_TUNIC_COLLECTED, {
        if (OotmmNpcChecks_IsShuffled("OOT_GORON_LINK_TUNIC")) {
            *should = OotmmNpcChecks_IsCollected("OOT_GORON_LINK_TUNIC");
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_GRANNYS_SHOP, {
        if (ConsumeLedgerRepeat() ||
            (!OotmmNpcChecks_IsCollected("OOT_WITCH_BLUE_POTION") && Collect("OOT_WITCH_BLUE_POTION"))) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_MAGIC_BEAN_SALESMAN, {
        if (Collect("OOT_BEAN_SELLER")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_MEDIGORON, {
        if (Collect("OOT_MEDIGORON")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_GIANTS_KNIFE_PURCHASE, {
        if (OotmmNpcChecks_IsShuffled("OOT_MEDIGORON")) {
            *should = !OotmmNpcChecks_IsCollected("OOT_MEDIGORON");
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_DEKU_THEATER, {
        EnDntJiji* jiji = va_arg(args, EnDntJiji*);
        if (jiji != nullptr && Collect(DekuTheaterSite(jiji->getItemId))) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_REVERT_SPOILING_ITEMS, {
        if (OotmmSession_IsActive()) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_FISHING_PRIZE, {
        const VBFishingData* data = va_arg(args, const VBFishingData*);
        const char* site = FishingSite();
        if (IsFishingPrizeCatch(data) && !OotmmNpcChecks_IsCollected(site) && Collect(site)) {
            *should = true;
        }
    });
    REGISTER_VB_SHOULD(VB_SHOULD_GIVE_VANILLA_FISHING_PRIZE, {
        const VBFishingData* data = va_arg(args, const VBFishingData*);
        const char* site = FishingSite();
        if (OotmmNpcChecks_IsShuffled(site)) {
            *should = IsFishingPrizeCatch(data) && !OotmmNpcChecks_IsCollected(site);
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_GLITCH_FISHING_PRIZE, {
        if (OotmmNpcChecks_IsShuffled(FishingSite())) {
            *should = true;
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_CARPET_SALESMAN, {
        if (ConsumeLedgerRepeat() ||
            (!OotmmNpcChecks_IsCollected("OOT_CARPET_MERCHANT") && Collect("OOT_CARPET_MERCHANT"))) {
            *should = true;
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_POE_COLLECTOR, {
        if (Collect("OOT_POE_COLLECTOR")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_GERUDOS_BE_FRIENDLY, {
        if (OotmmSession_IsActive()) {
            *should = CHECK_QUEST_ITEM(QUEST_GERUDO_CARD);
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_HORSEBACK_ARCHERY, {
        EnGe1* gerudo = va_arg(args, EnGe1*);
        constexpr uint16_t kGiveQuiver = 1 << 1;
        if (gerudo != nullptr && Collect((gerudo->stateFlags & kGiveQuiver) ? "OOT_GERUDO_ARCHERY_2"
                                                                           : "OOT_GERUDO_ARCHERY_1")) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_LOST_DOG, {
        if (Collect("OOT_DOG_LADY")) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_SPAWN_FIRE_ARROW, {
        if (OotmmNpcChecks_IsShuffled("OOT_FIRE_ARROW")) {
            *should = !OotmmNpcChecks_IsCollected("OOT_FIRE_ARROW");
        }
    });

    REGISTER_VB_SHOULD(VB_SPAWN_LW_FADO, {
        *should = OwnsTradeItem(SLOT_TRADE_ADULT, ITEM_ODD_POTION) &&
                  (gSaveContext.inventory.items[SLOT_TRADE_ADULT] != ITEM_COJIRO);
    });
    REGISTER_VB_SHOULD(VB_DESPAWN_GROG, {
        *should = !((OwnsTradeItem(SLOT_TRADE_ADULT, ITEM_COJIRO) &&
                     !OwnsTradeItem(SLOT_TRADE_ADULT, ITEM_ODD_POTION)) ||
                    (gSaveContext.inventory.items[SLOT_TRADE_ADULT] == ITEM_COJIRO));
    });

    REGISTER_VB_SHOULD(VB_BE_ELIGIBLE_FOR_DARUNIAS_JOY_REWARD, {
        if (OotmmNpcChecks_IsShuffled("OOT_DARUNIA_BRACELET")) {
            *should = !OotmmNpcChecks_IsCollected("OOT_DARUNIA_BRACELET");
        }
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_STRENGTH_1, {
        if (Collect("OOT_DARUNIA_BRACELET")) {
            *should = false;
        }
    });

    REGISTER_VB_SHOULD(VB_GORONS_CONSIDER_DODONGOS_CAVERN_FINISHED, {
        *should = Flags_GetEventChkInf(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP);
    });
    REGISTER_VB_SHOULD(VB_GORONS_CONSIDER_FIRE_TEMPLE_FINISHED, {
        *should = Flags_GetEventChkInf(EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP);
    });
    REGISTER_VB_SHOULD(VB_BIGGORON_CONSIDER_TRADE_COMPLETE, {
        if (OotmmSession_IsActive()) {
            *should = false;
        }
    });
    REGISTER_VB_SHOULD(VB_BIGGORON_CONSIDER_SWORD_COLLECTED, {
        if (OotmmNpcChecks_IsShuffled("OOT_TRADE_BIGGORON_SWORD")) {
            *should = OotmmNpcChecks_IsCollected("OOT_TRADE_BIGGORON_SWORD");
        }
    });

    REGISTER_VB_SHOULD(VB_DEKU_JR_CONSIDER_FOREST_TEMPLE_FINISHED, {
        *should = Flags_GetEventChkInf(EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP);
    });
    REGISTER_VB_SHOULD(VB_KING_ZORA_THANK_CHILD, {
        *should = Flags_GetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP);
    });
    REGISTER_VB_SHOULD(VB_KING_ZORA_TUNIC_CHECK, {
        if (OotmmNpcChecks_IsShuffled("OOT_ZORA_KING_TUNIC")) {
            *should = OotmmNpcChecks_IsCollected("OOT_ZORA_KING_TUNIC");
        }
    });

    REGISTER_VB_SHOULD(VB_BE_ABLE_TO_PLAY_BOMBCHU_BOWLING, {
        *should = CUR_UPG_VALUE(UPG_BOMB_BAG) > 0 || OwnsBombchuBag();
    });
    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_BOMBCHU_BOWLING, {
        EnBomBowlPit* pit = va_arg(args, EnBomBowlPit*);
        if (pit != nullptr && Collect(BowlingSite(pit->prizeIndex))) {
            *should = false;
        }
    });
}
