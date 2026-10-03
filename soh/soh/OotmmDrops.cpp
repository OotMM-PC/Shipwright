#include "OotmmDrops.h"

#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmIpc.h"
#include "OotmmItemModels.h"
#include "OotmmItemPresentation.h"
#include "OotmmSession.h"

#include <random>
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

struct Drop {
    std::string ItemId;
    std::string ItemName;
    std::string ItemGame;
    std::string CheckId;
    // What the drop draws as; differs from the item itself for disguised traps.
    std::string DrawItemId;
    std::string DrawItemName;
};

// A trap must not advertise itself: disguise it as a random major item from the seed.
void PickDisguise(std::string& drawItemId, std::string& drawItemName) {
    std::vector<const Ship::OotmmPlacement*> majors;
    for (const auto& placement : OotmmSession_GetState().GetPlacements()) {
        if (placement.Major && !placement.ItemId.empty() &&
            !Ship::OotmmItemCatalog::IsTrap(placement.ItemId)) {
            majors.push_back(&placement);
        }
    }
    if (majors.empty()) {
        return;
    }
    static std::mt19937 sRng{ std::random_device{}() };
    const auto* pick = majors[std::uniform_int_distribution<size_t>(0, majors.size() - 1)(sRng)];
    drawItemId = pick->ItemId;
    drawItemName = pick->ItemName;
}

// Spawned drop -> its item; entries whose actors die without a pickup are pruned.
std::unordered_map<Actor*, Drop> sDrops;

// Item_DropCollectible's own starting countdown; refreshed every frame so drops never blink out.
constexpr s16 kDespawnRefresh = 220;
constexpr float kWorldScale = 0.25f;

void Present(const Drop& drop) {
    Ship::GameIpcItemPresentation presentation;
    presentation.ItemId = drop.ItemId;
    presentation.ItemName = drop.ItemName;
    presentation.ItemGame = drop.ItemGame;
    presentation.DestinationGame = drop.ItemGame;
    const uint32_t self = OotmmSession_GetState().GetPlayerId();
    presentation.SourcePlayer = self;
    presentation.RecipientPlayer = self;
    presentation.ViewerPlayer = self;
    presentation.Debug = drop.CheckId.empty();
    OotmmItemPresentation_Queue(std::move(presentation));
}

} // namespace

extern "C" void OotmmDrops_DrawActor(Actor* actor, PlayState* play) {
    const auto it = sDrops.find(actor);
    if (it == sDrops.end()) {
        return;
    }
    if (actor->scale.x > 0.0001f) {
        const float scale = kWorldScale / actor->scale.x;
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    }
    if (OotmmItemModel_DrawById(play, it->second.DrawItemId, it->second.DrawItemName)) {
        return;
    }
    GetItem_Draw(play, GID_RUPEE_GREEN);
}

bool OotmmDrops_Spawn(PlayState* play, float x, float y, float z, const std::string& itemId,
                      const std::string& itemName, const std::string& itemGame,
                      const std::string& checkId) {
    if (play == nullptr) {
        return false;
    }
    Vec3f pos = { x, y, z };
    EnItem00* item = Item_DropCollectible(play, &pos, ITEM00_RUPEE_GREEN);
    if (item == nullptr) {
        return false;
    }
    item->actor.draw = OotmmDrops_DrawActor;
    item->unk_15A = kDespawnRefresh;
    std::string drawItemId = itemId;
    std::string drawItemName = itemName;
    if (Ship::OotmmItemCatalog::IsTrap(itemId)) {
        PickDisguise(drawItemId, drawItemName);
    }
    sDrops.emplace(&item->actor, Drop{ itemId, itemName, itemGame, checkId, drawItemId, drawItemName });
    return true;
}

void OotmmDrops_Init() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) { sDrops.clear(); });

    REGISTER_VB_SHOULD(VB_GIVE_ITEM_FROM_ITEM_00, {
        EnItem00* item = va_arg(args, EnItem00*);
        const auto it = sDrops.find(&item->actor);
        if (it != sDrops.end()) {
            Present(it->second);
            if (!it->second.CheckId.empty() && !OotmmIpc_IsCheckCompleted(it->second.CheckId)) {
                OotmmIpc_SendCheckCollected(it->second.CheckId);
            }
            Actor_Kill(&item->actor);
            sDrops.erase(it);
            *should = false;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([]() {
        if (gPlayState == nullptr || sDrops.empty()) {
            return;
        }
        std::unordered_map<Actor*, Drop> alive;
        for (Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_MISC].head; actor != nullptr;
             actor = actor->next) {
            const auto it = sDrops.find(actor);
            if (it != sDrops.end()) {
                reinterpret_cast<EnItem00*>(actor)->unk_15A = kDespawnRefresh;
                alive.emplace(actor, std::move(it->second));
            }
        }
        sDrops = std::move(alive);
    });
}
