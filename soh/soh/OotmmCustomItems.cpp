#include "OotmmCustomItems.h"

#include "Enhancements/custom-message/CustomMessageManager.h"
#include "Enhancements/game-interactor/GameInteractor.h"
#include "OotmmIpc.h"
#include "OotmmItemPresentation.h"
#include "ResourceManagerHelpers.h"
#include "OotmmSession.h"

#include <array>
#include <cstring>
#include <string>
#include <string_view>

extern "C" {
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "z64.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"

#include "OotmmCustomItemsPlayer.h"

extern PlayState* gPlayState;
}

namespace {

struct CustomItem {
    uint8_t Item;
    std::string_view OotmmId;
    std::string_view SharedId;
    const char* Icon;
    const char* NameTexture;
    const char* DisplayName;
    int32_t Action;
    const char* MaskDList;
    const char* SpentMaskDList;
    const char* BLabel;
};

constexpr std::array<CustomItem, OOTMM_CUSTOM_ITEM_COUNT> kItems = { {
    { ITEM_OOTMM_MASK_BLAST, "OOT_MASK_BLAST", "SHARED_MASK_BLAST",
      "__OTR__mm_icon_item_static_yar/gItemIconBlastMaskTex",
      "__OTR__mm_item_name_static/gItemNameBlastMaskENGTex", "Blast Mask", PLAYER_IA_MASK_OOTMM_BLAST,
      "__OTR__objects/mm_obj_mask_bakuretu/object_mask_bakuretu_DL_0005C0",
      "__OTR__objects/mm_obj_mask_bakuretu/object_mask_bakuretu_DL_000440",
      "__OTR__mm_do_action_static/gDoActionExplodeENGTex" },
    { ITEM_OOTMM_MASK_STONE, "OOT_MASK_STONE", "SHARED_MASK_STONE",
      "__OTR__mm_icon_item_static_yar/gItemIconStoneMaskTex",
      "__OTR__mm_item_name_static/gItemNameStoneMaskENGTex", "Stone Mask", PLAYER_IA_MASK_OOTMM_STONE,
      "__OTR__objects/mm_obj_mask_stone/object_mask_stone_DL_000820", nullptr, nullptr },
    { ITEM_OOTMM_MASK_KAMARO, "OOT_MASK_KAMARO", "SHARED_MASK_KAMARO",
      "__OTR__mm_icon_item_static_yar/gItemIconKamaroMaskTex",
      "__OTR__mm_item_name_static/gItemNameKamarosMaskENGTex", "Kamaro's Mask",
      PLAYER_IA_MASK_OOTMM_KAMARO,
      "__OTR__objects/mm_obj_mask_dancer/object_mask_dancer_DL_000EF0", nullptr,
      "__OTR__mm_do_action_static/gDoActionDanceENGTex" },
    { ITEM_OOTMM_POWDER_KEG, "OOT_POWDER_KEG", "SHARED_POWDER_KEG",
      "__OTR__mm_icon_item_static_yar/gItemIconPowderKegTex",
      "__OTR__mm_item_name_static/gItemNamePowderKegENGTex", "Powder Keg", PLAYER_IA_OOTMM_POWDER_KEG,
      nullptr , nullptr, nullptr },
    { ITEM_OOTMM_GREAT_FAIRY_SWORD, "OOT_GREAT_FAIRY_SWORD", "SHARED_GREAT_FAIRY_SWORD",
      "__OTR__mm_icon_item_static_yar/gItemIconGreatFairysSwordTex",
      "__OTR__mm_item_name_static/gItemNameGreatFairysSwordENGTex", "Great Fairy's Sword",
      PLAYER_IA_SWORD_OOTMM_GREAT_FAIRY, nullptr , nullptr, nullptr },
} };

const CustomItem* FindByMask(uint8_t mask) {
    for (const auto& entry : kItems) {
        if (entry.MaskDList != nullptr && entry.Action - PLAYER_IA_MASK_KEATON + 1 == mask) {
            return &entry;
        }
    }
    return nullptr;
}

const CustomItem* Find(uint8_t item) {
    for (const auto& entry : kItems) {
        if (entry.Item == item) {
            return &entry;
        }
    }
    return nullptr;
}

uint32_t InventoryCount(const CustomItem& entry) {
    const auto& inventory = OotmmIpc_GetInventory();
    const uint32_t own = inventory.Count(std::string(entry.OotmmId));
    if (own != 0) {
        return own;
    }
    return entry.SharedId.empty() ? 0 : inventory.Count(std::string(entry.SharedId));
}

} // namespace

extern "C" int OotmmCustomItems_IsCustomItem(uint8_t item) {
    return item >= ITEM_OOTMM_FIRST && item < ITEM_OOTMM_MAX ? 1 : 0;
}

extern "C" int OotmmCustomItems_IsCustomSlot(uint8_t slot) {
    return slot >= SLOT_OOTMM_FIRST && slot < SLOT_OOTMM_MAX ? 1 : 0;
}

extern "C" uint8_t OotmmCustomItems_SlotOf(uint8_t item) {
    if (!OotmmCustomItems_IsCustomItem(item)) {
        return SLOT_NONE;
    }
    return static_cast<uint8_t>(SLOT_OOTMM_FIRST + (item - ITEM_OOTMM_FIRST));
}

extern "C" uint8_t OotmmCustomItems_ItemInSlot(uint8_t slot) {
    if (!OotmmCustomItems_IsCustomSlot(slot)) {
        return ITEM_NONE;
    }
    return static_cast<uint8_t>(ITEM_OOTMM_FIRST + (slot - SLOT_OOTMM_FIRST));
}

extern "C" const char* OotmmCustomItems_IconPath(uint8_t item) {
    const CustomItem* entry = Find(item);
    return entry != nullptr ? entry->Icon : nullptr;
}

extern "C" void* OotmmCustomItems_ButtonIcon(uint8_t item) {
    const CustomItem* entry = Find(item);
    if (entry != nullptr) {
        return const_cast<char*>(entry->Icon);
    }
    // gItemIcons only covers vanilla ids; anything else would read past its end.
    return item < ARRAY_COUNT(gItemIcons) ? gItemIcons[item] : nullptr;
}

extern "C" const char* OotmmCustomItems_DisplayName(uint8_t item) {
    const CustomItem* entry = Find(item);
    return entry != nullptr ? entry->DisplayName : nullptr;
}

extern "C" uint8_t OotmmCustomItems_IdForItemId(const char* ootmmItemId) {
    if (ootmmItemId == nullptr) {
        return ITEM_NONE;
    }
    const std::string_view id = ootmmItemId;
    for (const auto& entry : kItems) {
        if (entry.OotmmId == id || (!entry.SharedId.empty() && entry.SharedId == id)) {
            return entry.Item;
        }
    }
    return ITEM_NONE;
}

extern "C" uint32_t OotmmCustomItems_OwnedCount(uint8_t item) {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const CustomItem* entry = Find(item);
    return entry != nullptr ? InventoryCount(*entry) : 0;
}

extern "C" int OotmmCustomItems_Owned(uint8_t item) {
    return OotmmCustomItems_OwnedCount(item) != 0 ? 1 : 0;
}

extern "C" int OotmmCustomItems_OwnedItemCount(void) {
    int count = 0;
    for (const auto& entry : kItems) {
        if (OotmmCustomItems_Owned(entry.Item)) {
            count++;
        }
    }
    return count;
}

extern "C" uint8_t OotmmCustomItems_OwnedItemAt(int index) {
    int seen = 0;
    for (const auto& entry : kItems) {
        if (!OotmmCustomItems_Owned(entry.Item)) {
            continue;
        }
        if (seen == index) {
            return entry.Item;
        }
        seen++;
    }
    return ITEM_NONE;
}

extern "C" int32_t OotmmCustomItems_ItemAction(uint8_t item) {
    const CustomItem* entry = Find(item);
    return entry != nullptr ? entry->Action : -1;
}

extern "C" const char* OotmmCustomItems_MaskDList(uint8_t mask) {
    const CustomItem* entry = FindByMask(mask);
    return entry != nullptr ? entry->MaskDList : nullptr;
}

extern "C" int OotmmCustomItems_AmmoOf(uint8_t item) {
    return item == ITEM_OOTMM_POWDER_KEG ? OotmmCustomItems_KegAmmo() : -1;
}

extern "C" uint8_t OotmmCustomItems_ButtonIconShade(uint8_t item) {
    return OotmmCustomItems_IsCustomItem(item) && !OotmmCustomItems_UsableNow(item) ? 100 : 255;
}

extern "C" int OotmmCustomItems_UsableNow(uint8_t item) {
    const Ship::OotmmGameState& state = OotmmSession_GetState();

    switch (item) {
        case ITEM_OOTMM_POWDER_KEG:
            return OotmmCustomItems_CanCarryKeg();
        case ITEM_OOTMM_GREAT_FAIRY_SWORD:
            return LINK_IS_CHILD || state.GetBoolSetting("agelessGFS", false);
        case ITEM_OOTMM_MASK_BLAST:
        case ITEM_OOTMM_MASK_STONE:
        case ITEM_OOTMM_MASK_KAMARO:
            return LINK_IS_CHILD || state.GetBoolSetting("agelessChildTrade", false);
        default:
            return 1;
    }
}

extern "C" int OotmmCustomItems_Usable(uint8_t item) {
    return OotmmCustomItems_Owned(item);
}

namespace {

void BuildMedigoronKegText(uint16_t*, bool* loadFromMessageTable) {
    if (!OotmmCustomItems_MedigoronSellsKeg()) {
        return;
    }

    CustomMessage message("If you don't have a %rPowder Keg%w,&I'll sell you one for %r50 Rupees%w!"
                          "So, how about it?	&&%gBuy&Don't buy%w");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BuildMedigoronRefusalText(uint16_t*, bool* loadFromMessageTable) {
    if (!OotmmCustomItems_Owned(ITEM_OOTMM_POWDER_KEG) || OotmmCustomItems_KegAmmo() == 0) {
        return;
    }

    CustomMessage message(OotmmCustomItems_CanCarryKeg()
                              ? "%rPowder Kegs%w are dangerous&explosives, so you can carry only&one at a time!"
                              : "Oh! But my product is so %rheavy%w, I&don't think you can carry it."
                                "I'm sorry I even brought it up...");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromMessageTable = false;
}

} // namespace

extern "C" void OotmmCustomItems_Init(void) {
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x304F, BuildMedigoronKegText);
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnOpenText>(0x304D, BuildMedigoronRefusalText);

    REGISTER_VB_SHOULD(VB_DRAW_CUSTOM_ITEM_NAME, {
        const int16_t item = static_cast<int16_t>(va_arg(args, int));
        const CustomItem* entry = Find(static_cast<uint8_t>(item));
        if (entry != nullptr && gPlayState != nullptr) {
            const size_t length = std::strlen(entry->NameTexture);
            std::memcpy(gPlayState->pauseCtx.nameSegment, entry->NameTexture, length + 1);
            *should = true;
        }
    });
}

namespace {

constexpr std::array<int16_t, 34> kStoneMaskBlindActors = { {
    ACTOR_EN_NY,      ACTOR_EN_SB,       ACTOR_EN_RR,      ACTOR_EN_TORCH2,  ACTOR_EN_FZ,
    ACTOR_EN_WEIYER,  ACTOR_EN_EIYER,    ACTOR_EN_BB,      ACTOR_EN_ANUBICE, ACTOR_EN_TP,
    ACTOR_EN_BA,      ACTOR_EN_GOMA,     ACTOR_EN_DODOJR,  ACTOR_EN_SKJ,     ACTOR_EN_CROW,
    ACTOR_EN_HINTNUTS, ACTOR_EN_WALLMAS, ACTOR_EN_REEBA,   ACTOR_EN_FLOORMAS, ACTOR_EN_DEKUNUTS,
    ACTOR_EN_DNS,     ACTOR_EN_MB,       ACTOR_EN_TITE,    ACTOR_EN_PEEHAT,  ACTOR_EN_FIREFLY,
    ACTOR_EN_DODONGO, ACTOR_EN_SW,       ACTOR_EN_VM,      ACTOR_EN_ST,      ACTOR_EN_GE1,
    ACTOR_EN_GE2,     ACTOR_EN_BILI,     ACTOR_EN_VALI,    ACTOR_EN_HEISHI2,
} };

// The blast mask display lists branch through segments 0x08 and 0x09; OoT binds neither.
Gfx sEmptyDList[] = {
    gsSPEndDisplayList(),
};

Mtx* sMaskMatrix = nullptr;
u8 sTunicEnvColor[3] = { 255, 255, 255 };

constexpr int16_t kBlastMaskFadeFrames = 0x11;
int16_t sBlastMaskTimer = 0;

} // namespace

extern "C" void OotmmCustomItems_SetTunicEnvColor(u8 r, u8 g, u8 b) {
    sTunicEnvColor[0] = r;
    sTunicEnvColor[1] = g;
    sTunicEnvColor[2] = b;
}

extern "C" Gfx* OotmmCustomItems_GreatFairySwordHand(PlayState* play, Gfx* handDList) {
    Gfx* list = static_cast<Gfx*>(Graph_Alloc(play->state.gfxCtx, 5 * sizeof(Gfx)));
    if (list == nullptr) {
        return handDList;
    }

    Gfx* head = list;
    gSPDisplayList(head++, handDList);
    gSPDisplayList(head++,
                   ResourceMgr_LoadGfxByName("__OTR__objects/mm_obj_link_child/gLinkHumanGreatFairysSwordDL"));
    gDPPipeSync(head++);
    gDPSetEnvColor(head++, sTunicEnvColor[0], sTunicEnvColor[1], sTunicEnvColor[2], 0);
    gSPEndDisplayList(head);
    return list;
}

extern "C" int OotmmCustomItems_ActorIgnoresPlayer(Actor* actor, Player* player) {
    if (actor == nullptr || player == nullptr || player->currentMask != PLAYER_MASK_OOTMM_STONE) {
        return 0;
    }
    if (actor->id == ACTOR_EN_AM) {
        return actor->params != 0 ? 1 : 0;
    }
    for (const int16_t id : kStoneMaskBlindActors) {
        if (actor->id == id) {
            return 1;
        }
    }
    return actor->id == ACTOR_EN_HEISHI3 || actor->id == ACTOR_EN_HEISHI4 ? 1 : 0;
}

extern "C" int OotmmCustomItems_TryBlastMask(PlayState* play, Player* player) {
    if (player->currentMask != PLAYER_MASK_OOTMM_BLAST) {
        return 0;
    }
    if (sBlastMaskTimer != 0) {
        return 1;
    }

    Actor* bomb = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BOM, player->actor.focus.pos.x,
                              player->actor.focus.pos.y, player->actor.focus.pos.z, 0, 0, 0, 0);
    if (bomb == nullptr) {
        return 1;
    }

    EnBom* explosive = reinterpret_cast<EnBom*>(bomb);

    explosive->timer = 0;
    // EnBom only centres its explosion sphere from EnBom_Draw, which an instant fuse skips.
    explosive->explosionCollider.elements[0].dim.worldSphere.center.x =
        static_cast<s16>(bomb->world.pos.x);
    explosive->explosionCollider.elements[0].dim.worldSphere.center.y =
        static_cast<s16>(bomb->world.pos.y);
    explosive->explosionCollider.elements[0].dim.worldSphere.center.z =
        static_cast<s16>(bomb->world.pos.z);

    sBlastMaskTimer = static_cast<int16_t>(OotmmSession_GetState().GetBlastMaskCooldownFrames());
    return 1;
}

extern "C" void OotmmCustomItems_CaptureMaskMatrix(PlayState* play, Player* player) {
    // Dark Link borrows this limb callback with a Player struct he never initialises.
    if (player != GET_PLAYER(play) || OotmmCustomItems_MaskDList(player->currentMask) == nullptr) {
        return;
    }
    sMaskMatrix = MATRIX_NEWMTX(play->state.gfxCtx);
}

extern "C" void OotmmCustomItems_DrawWornMask(PlayState* play, Player* player) {
    const CustomItem* entry = FindByMask(player->currentMask);
    Mtx* matrix = sMaskMatrix;

    sMaskMatrix = nullptr;
    if (entry == nullptr || matrix == nullptr) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    // Drawn after the skeleton so the mask's envelope colour cannot reach Link's body.
    gSPMatrix(POLY_OPA_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPSegment(POLY_OPA_DISP++, 0x09, reinterpret_cast<uintptr_t>(sEmptyDList));

    const uint8_t opacity =
        entry->SpentMaskDList == nullptr || sBlastMaskTimer <= kBlastMaskFadeFrames
            ? static_cast<uint8_t>(255 - sBlastMaskTimer * 0x0F)
            : 0;

    if (opacity != 0) {
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, opacity);
        gSPDisplayList(POLY_OPA_DISP++, ResourceMgr_LoadGfxByName(entry->MaskDList));
    } else {
        gSPSegment(POLY_OPA_DISP++, 0x08, reinterpret_cast<uintptr_t>(sEmptyDList));
        gDPSetEnvColor(POLY_OPA_DISP++, 0, 0, 0, 255);
        gSPDisplayList(POLY_OPA_DISP++, ResourceMgr_LoadGfxByName(entry->SpentMaskDList));
    }

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetEnvColor(POLY_OPA_DISP++, sTunicEnvColor[0], sTunicEnvColor[1], sTunicEnvColor[2], 0);

    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void OotmmCustomItems_MarkExplosive(Actor* spawned, int explosiveType) {
    // EnBom grows its explosion by (shape.rot.z + 8) per frame.
    spawned->shape.rot.z = explosiveType == 2 ? 12 : 0;
    EnBom* bomb = reinterpret_cast<EnBom*>(spawned);

    bomb->isPowderKeg = explosiveType == 2 ? 1 : 0;
    if (explosiveType == 2) {
        bomb->explosionColliderItems[0].info.toucher.dmgFlags |= DMG_HAMMER_JUMP;
    }
    if (explosiveType == 2 && spawned->scale.x != 0.0f) {
        Actor_SetScale(spawned, OotmmCustomItems_ExplosiveScale(spawned));
    }
}

extern "C" float OotmmCustomItems_ExplosiveScale(Actor* actor) {
    return reinterpret_cast<EnBom*>(actor)->isPowderKeg ? 0.03f : 0.01f;
}

extern "C" int OotmmCustomItems_DrawPowderKeg(PlayState* play, Actor* actor) {
    if (!reinterpret_cast<EnBom*>(actor)->isPowderKeg || actor->params != BOMB_BODY) {
        return 0;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    func_8002EBCC(actor, play, 0);
    gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gSPDisplayList(POLY_OPA_DISP++,
                   ResourceMgr_LoadGfxByName("__OTR__overlays/mm_ovl_En_Bom/gPowderKegBarrelDL"));
    gSPDisplayList(POLY_OPA_DISP++,
                   ResourceMgr_LoadGfxByName("__OTR__overlays/mm_ovl_En_Bom/gPowderKegGoronSkullDL"));

    CLOSE_DISPS(play->state.gfxCtx);
    return 1;
}

extern "C" const char* OotmmCustomItems_KamaroDanceAnim(void) {
    return "__OTR__objects/mm_gameplay_keep/gPlayerAnim_alink_dance_loop";
}

extern "C" void* OotmmCustomItems_BButtonIcon(uint8_t item) {
    if (gPlayState != nullptr && GET_PLAYER(gPlayState)->rideActor == NULL) {
        const CustomItem* worn = FindByMask(GET_PLAYER(gPlayState)->currentMask);
        if (worn != nullptr && worn->Item != ITEM_OOTMM_MASK_STONE) {
            return const_cast<char*>(worn->Icon);
        }
    }
    return OotmmCustomItems_ButtonIcon(item);
}

extern "C" const char* OotmmCustomItems_BButtonLabel(void) {
    if (gPlayState != nullptr && GET_PLAYER(gPlayState)->rideActor == NULL) {
        const CustomItem* worn = FindByMask(GET_PLAYER(gPlayState)->currentMask);
        if (worn != nullptr) {
            return worn->BLabel;
        }
    }
    return nullptr;
}

extern "C" uint8_t OotmmCustomItems_BButtonShade(void) {
    if (gPlayState != nullptr && GET_PLAYER(gPlayState)->currentMask == PLAYER_MASK_OOTMM_BLAST &&
        sBlastMaskTimer != 0) {
        return 100;
    }
    return 255;
}

extern "C" void OotmmCustomItems_TickTimers(void) {
    if (sBlastMaskTimer != 0) {
        sBlastMaskTimer--;
    }
}

extern "C" int OotmmCustomItems_KegAmmo(void) {
    return gSaveContext.ship.ootmmKegAmmo;
}

extern "C" void OotmmCustomItems_SetKegAmmo(int ammo) {
    gSaveContext.ship.ootmmKegAmmo = static_cast<u8>(ammo < 0 ? 0 : (ammo > 1 ? 1 : ammo));
}

extern "C" int OotmmCustomItems_CanCarryKeg(void) {
    return CUR_UPG_VALUE(UPG_STRENGTH) >= 3 ? 1 : 0;
}

extern "C" void OotmmCustomItems_UpdateKegStock(void) {
    if (OotmmCustomItems_Owned(ITEM_OOTMM_POWDER_KEG) && gSaveContext.ship.ootmmKegGranted == 0) {
        gSaveContext.ship.ootmmKegGranted = 1;
        OotmmCustomItems_SetKegAmmo(1);
    }
}

extern "C" int OotmmCustomItems_MedigoronKegPrice(void) {
    return 50;
}

extern "C" int OotmmCustomItems_MedigoronSellsKeg(void) {
    // TODO: the Giant's Knife is randomized, so owning it does not mean this check was completed;
    // this needs to ask the check system whether Medigoron's check is done before selling kegs.
    return CHECK_OWNED_EQUIP_ALT(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON) &&
                   OotmmCustomItems_Owned(ITEM_OOTMM_POWDER_KEG) && OotmmCustomItems_KegAmmo() == 0
               ? 1
               : 0;
}

extern "C" void OotmmCustomItems_BuyKegFromMedigoron(void) {
    Rupees_ChangeBy(-OotmmCustomItems_MedigoronKegPrice());
    OotmmCustomItems_SetKegAmmo(1);

    Ship::GameIpcItemPresentation presentation;
    presentation.ItemId = "OOT_POWDER_KEG";
    presentation.ItemName = "Powder Keg";
    presentation.ItemGame = "oot";
    presentation.DestinationGame = "oot";
    OotmmItemPresentation_Queue(presentation);
}

extern "C" int OotmmCustomItems_ExplosiveAmmo(int explosiveType) {
    if (explosiveType != 2) {
        return 0;
    }
    return OotmmCustomItems_CanCarryKeg() ? OotmmCustomItems_KegAmmo() : 0;
}
