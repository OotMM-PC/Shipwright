#include "OotmmShops.h"

#include "Enhancements/custom-message/CustomMessageManager.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmCheckLedger.h"
#include "OotmmIpc.h"
#include "OotmmItemModels.h"
#include "OotmmItemPresentation.h"
#include "OotmmNpcChecks.h"
#include "OotmmSession.h"

#include <string>
#include <unordered_map>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "src/overlays/actors/ovl_En_GirlA/z_en_girla.h"
#include "src/overlays/actors/ovl_En_Ossan/z_en_ossan.h"

extern PlayState* gPlayState;
}

namespace {

enum ShopState : uint8_t {
    kVanilla,
    kShuffled,
    kRenewable,
    kSoldOut,
};

int ShopId(const EnGirlA* item, const PlayState* play) {
    const int params = item->actor.params;
    switch (play->sceneNum) {
        case SCENE_KOKIRI_SHOP:
            switch (params) {
                case SI_RECOVERY_HEART: return 0x00;
                case SI_ARROWS_10: return 0x01;
                case SI_ARROWS_30: return 0x02;
                case SI_DEKU_SEEDS_30: return 0x03;
                case SI_DEKU_NUTS_5: return 0x04;
                case SI_DEKU_NUTS_10: return 0x05;
                case SI_DEKU_STICK: return 0x06;
                case SI_DEKU_SHIELD: return 0x07;
                default: return -1;
            }
        case SCENE_BOMBCHU_SHOP:
            switch (params) {
                case SI_BOMBCHU_20_1: return 0x08;
                case SI_BOMBCHU_20_2: return 0x09;
                case SI_BOMBCHU_20_3: return 0x0a;
                case SI_BOMBCHU_20_4: return 0x0b;
                case SI_BOMBCHU_10_1: return 0x0c;
                case SI_BOMBCHU_10_2: return 0x0d;
                case SI_BOMBCHU_10_3: return 0x0e;
                case SI_BOMBCHU_10_4: return 0x0f;
                default: return -1;
            }
        case SCENE_ZORA_SHOP:
            switch (params) {
                case SI_RED_POTION_R50: return 0x10;
                case SI_ARROWS_50: return 0x11;
                case SI_FISH: return 0x12;
                case SI_DEKU_NUTS_5: return 0x13;
                case SI_ARROWS_10: return 0x14;
                case SI_ARROWS_30: return 0x15;
                case SI_ZORA_TUNIC: return 0x16;
                case SI_RECOVERY_HEART: return 0x17;
                default: return -1;
            }
        case SCENE_GORON_SHOP:
            switch (params) {
                case SI_RECOVERY_HEART: return item->actor.world.pos.x < -60.0f ? 0x18 : 0x19;
                case SI_RED_POTION_R40: return 0x1a;
                case SI_GORON_TUNIC: return 0x1b;
                case SI_BOMBS_5_R25: return 0x1c;
                case SI_BOMBS_10: return 0x1d;
                case SI_BOMBS_20: return 0x1e;
                case SI_BOMBS_30: return 0x1f;
                default: return -1;
            }
        case SCENE_BAZAAR: {
            const int kakariko = gEntranceTable[gSaveContext.entranceIndex].spawn == 1 ? 0x10 : 0x00;
            switch (params) {
                case SI_ARROWS_10: return 0x20 | kakariko;
                case SI_ARROWS_30: return 0x21 | kakariko;
                case SI_ARROWS_50: return 0x22 | kakariko;
                case SI_DEKU_STICK: return 0x23 | kakariko;
                case SI_BOMBS_5_R35: return 0x24 | kakariko;
                case SI_RECOVERY_HEART: return 0x25 | kakariko;
                case SI_DEKU_NUTS_5: return 0x26 | kakariko;
                case SI_HYLIAN_SHIELD: return 0x27 | kakariko;
                default: return -1;
            }
        }
        case SCENE_POTION_SHOP_MARKET:
        case SCENE_POTION_SHOP_KAKARIKO: {
            const int kakariko = play->sceneNum == SCENE_POTION_SHOP_KAKARIKO ? 0x10 : 0x00;
            switch (params) {
                case SI_FISH: return 0x28 | kakariko;
                case SI_BUGS: return 0x29 | kakariko;
                case SI_POE: return 0x2a | kakariko;
                case SI_DEKU_NUTS_5: return 0x2b | kakariko;
                case SI_BLUE_FIRE: return 0x2c | kakariko;
                case SI_FAIRY: return 0x2d | kakariko;
                case SI_GREEN_POTION: return 0x2e | kakariko;
                case SI_RED_POTION_R30: return 0x2f | kakariko;
                default: return -1;
            }
        }
        default:
            return -1;
    }
}

const Ship::OotmmPlacement* FindShop(const EnGirlA* item) {
    if (item->ootmmState == kVanilla) {
        return nullptr;
    }
    return OotmmSession_GetState().FindCheck(Ship::OotmmGame::Oot, "shop", std::to_string(item->ootmmShopId));
}

bool IsCollected(const Ship::OotmmPlacement& placement) {
    return OotmmCheckLedger_Contains(placement.CheckId) || OotmmIpc_IsCheckCompleted(placement.CheckId);
}

s32 CanBuyWith(EnGirlA* item, s32 success) {
    const auto* placement = FindShop(item);
    switch (placement != nullptr ? OotmmNpcChecks_SaleResult(*placement, false, item->basePrice) : 1) {
        case 0:
            return CANBUY_RESULT_NEED_RUPEES;
        case 1:
            return CANBUY_RESULT_CANT_GET_NOW;
        default:
            return success;
    }
}

s32 CanBuyShuffled(PlayState*, EnGirlA* item) {
    return CanBuyWith(item, CANBUY_RESULT_SUCCESS_FANFARE);
}

s32 CanBuyRenewal(PlayState*, EnGirlA* item) {
    return CanBuyWith(item, CANBUY_RESULT_SUCCESS);
}

void GiveRenewal(PlayState* play, EnGirlA* item) {
    const auto* placement = FindShop(item);
    if (const auto* repeat = placement != nullptr ? OotmmNpcChecks_FindRepeat(*placement) : nullptr) {
        OotmmNpcChecks_GiveRepeat(*repeat);
    }
    Rupees_ChangeBy(-item->basePrice);
}

void Noop(PlayState*, EnGirlA*) {
}

void ApplyCollected(EnGirlA* item, const Ship::OotmmPlacement& placement) {
    if (OotmmNpcChecks_FindRepeat(placement) != nullptr) {
        item->ootmmState = kRenewable;
        item->canBuyFunc = CanBuyRenewal;
        item->itemGiveFunc = GiveRenewal;
        item->buyEventFunc = Noop;
        return;
    }
    item->ootmmState = kSoldOut;
    item->isInvisible = true;
    item->actor.draw = nullptr;
    item->setOutOfStockFunc = Noop;
    item->updateStockedItemFunc = Noop;
}

void FinishPurchase(PlayState*, EnGirlA* item) {
    Rupees_ChangeBy(-item->basePrice);
    if (const auto* placement = FindShop(item)) {
        ApplyCollected(item, *placement);
    }
}

EnGirlA* SelectedItem(PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (actor->id == ACTOR_EN_OSSAN) {
            auto* ossan = reinterpret_cast<EnOssan*>(actor);
            return ossan->cursorIndex < 8 ? ossan->shelfSlots[ossan->cursorIndex] : nullptr;
        }
    }
    return nullptr;
}

void BuildShopText(uint16_t* textId, bool* loadFromMessageTable) {
    if (gPlayState == nullptr) {
        return;
    }
    EnGirlA* item = SelectedItem(gPlayState);
    if (item == nullptr || item->ootmmState == kVanilla || item->ootmmState == kSoldOut) {
        return;
    }
    const bool description = *textId == item->actor.textId;
    if (!description && *textId != item->itemBuyPromptTextId) {
        return;
    }
    const auto* placement = FindShop(item);
    if (placement == nullptr) {
        return;
    }
    const auto* repeat = item->ootmmState == kRenewable ? OotmmNpcChecks_FindRepeat(*placement) : nullptr;
    const std::string name = repeat != nullptr ? "%g" + std::string(repeat->Name) + "%w" : OotmmNpcChecks_PlacementName(*placement);
    std::string text = name + "&%r" + std::to_string(item->basePrice) + " Rupees%w";
    if (description) {
        text += "\x0A";
    } else {
        text += "&" + CustomMessage::TWO_WAY_CHOICE() + "%gBuy&No thanks%w";
    }
    CustomMessage message(text);
    message.Format();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

} // namespace

extern "C" void OotmmShops_SetupItem(EnGirlA* item, PlayState* play) {
    if (item == nullptr || play == nullptr || !OotmmSession_IsActive()) {
        return;
    }
    const int id = ShopId(item, play);
    if (id < 0) {
        return;
    }
    item->ootmmShopId = static_cast<s16>(id);
    item->ootmmState = kShuffled;
    const auto* placement = FindShop(item);
    if (placement == nullptr) {
        item->ootmmState = kVanilla;
        return;
    }
    item->basePrice = static_cast<s16>(OotmmSession_GetState().GetPrice(id).value_or(item->basePrice));
    if (IsCollected(*placement)) {
        ApplyCollected(item, *placement);
        return;
    }
    item->canBuyFunc = CanBuyShuffled;
    item->itemGiveFunc = Noop;
    item->buyEventFunc = FinishPurchase;
}

extern "C" int OotmmShops_IsShuffled(const EnGirlA* item) {
    return item != nullptr && item->ootmmState == kShuffled;
}

extern "C" int OotmmShops_Draw(EnGirlA* item, PlayState* play) {
    const auto* placement = item != nullptr && item->ootmmState != kVanilla ? FindShop(item) : nullptr;
    if (placement == nullptr) {
        return 0;
    }
    const bool cloaked = !placement->CloakItemId.empty();
    return OotmmItemModel_DrawById(play, cloaked ? placement->CloakItemId : placement->ItemId,
                                   cloaked ? placement->CloakItemName : placement->ItemName)
               ? 1
               : 0;
}

extern "C" int OotmmShops_Report(EnGirlA* item) {
    const auto* placement = OotmmShops_IsShuffled(item) ? FindShop(item) : nullptr;
    if (placement == nullptr) {
        return 0;
    }
    if (!IsCollected(*placement)) {
        OotmmIpc_SendCheckCollected(placement->CheckId, true);
        OotmmCheckLedger_Record(placement->CheckId);
    }
    return 1;
}

extern "C" int OotmmShops_InTransaction(PlayState* play) {
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_NPC].head; actor != nullptr; actor = actor->next) {
        if (actor->id == ACTOR_EN_OSSAN && reinterpret_cast<EnOssan*>(actor)->stateFlag != OSSAN_STATE_IDLE) {
            return 1;
        }
    }
    return 0;
}

extern "C" int OotmmShops_ContinuePresentation(EnGirlA* item, PlayState* play, Player* player) {
    const auto* placement = FindShop(item);
    if (placement == nullptr) {
        return 0;
    }
    return OotmmItemPresentation_IsDelivering(placement->CheckId) ||
                   OotmmItemPresentation_ContinueChest(play, player, placement->CheckId)
               ? 1
               : 0;
}

void OotmmShops_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(BuildShopText);
}
