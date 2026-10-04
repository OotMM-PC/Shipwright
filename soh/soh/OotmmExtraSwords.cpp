#include "OotmmExtraSwords.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include <algorithm>

extern "C" {
#include "z64.h"
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "soh/OotmmCustomItemsPlayer.h"
#include "objects/object_link_child/object_link_child.h"

extern PlayState* gPlayState;
}

namespace {

constexpr int kSlashKokiri = 8;
constexpr int kSlashGiant = 10;
constexpr int kSpinKokiri = 22;
constexpr int kJumpKokiri = 25;
constexpr s32 kDmgSlashKokiri = 1 << kSlashKokiri;
constexpr s32 kDmgSpinKokiri = 1 << kSpinKokiri;
constexpr s32 kDmgJumpKokiri = 1 << kJumpKokiri;

const char* const kRazorHand[] = { "__OTR__objects/mm_gameplay_keep/gRazorSwordDL" };
const char* const kGildedHand[] = { "__OTR__objects/mm_obj_link_child/gLinkHumanGildedSwordHandleDL",
                                    "__OTR__objects/mm_obj_link_child/gLinkHumanGildedSwordBladeDL" };
const char* const kSheathed[] = { "__OTR__objects/mm_obj_link_child/gLinkHumanSheathedRazorSwordDL",
                                  "__OTR__objects/mm_obj_link_child/gLinkHumanSheathedGildedSwordDL" };
const char* const kSheathEmpty[] = { "__OTR__objects/mm_obj_link_child/gLinkHumanRazorSwordSheathDL",
                                     "__OTR__objects/mm_obj_link_child/gLinkHumanGildedSwordSheathDL" };
const char* const kIcons[] = { "__OTR__mm_icon_item_static_yar/gItemIconRazorSwordTex",
                               "__OTR__mm_icon_item_static_yar/gItemIconGildedSwordTex" };

int Level() {
    if (!OotmmSession_IsActive()) {
        return 0;
    }
    const auto& state = OotmmSession_GetState();
    if (!state.GetBoolSetting("extraChildSwordsOot", false)) {
        return 0;
    }
    const char* item = state.GetBoolSetting("sharedSwords", false) ? "SHARED_SWORD" : "OOT_SWORD";
    const uint32_t count = OotmmIpc_GetInventory().Count(item);
    return static_cast<int>(std::clamp<uint32_t>(count, 1, 3) - 1);
}

bool KokiriUpgraded() {
    return Level() > 0 && CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_KOKIRI;
}

int Damage(const DamageTable* table, int type) {
    return table->table[type] & 0xF;
}

int Effect(const DamageTable* table, int type) {
    return table->table[type] >> 4;
}

void SetDamage(DamageTable* table, int type, int value) {
    table->table[type] = static_cast<u8>((table->table[type] & 0xF0) | (value & 0xF));
}

void SetEffect(DamageTable* table, int type, int value) {
    table->table[type] = static_cast<u8>((table->table[type] & 0xF) | (value << 4));
}

void UpdateDamageTable(void* actorPtr, bool*) {
    if (!OotmmSession_IsActive()) {
        return;
    }
    Actor* actor = static_cast<Actor*>(actorPtr);
    DamageTable* table = actor->colChkInfo.damageTable;
    if (table == nullptr || Damage(table, kSlashGiant) != 4) {
        return;
    }
    const int level = Level();
    if (Damage(table, kSlashKokiri) == 0 && level == 0) {
        return;
    }
    int damage = 1 + level;
    if (damage < 2 && (actor->id == ACTOR_EN_DH || actor->id == ACTOR_EN_DHA)) {
        damage = 2;
    }
    const int kokiriEffect = Effect(table, kSlashKokiri);
    const int giantEffect = Effect(table, kSlashGiant);
    SetDamage(table, kSlashKokiri, damage);
    SetDamage(table, kSpinKokiri, damage);
    SetDamage(table, kJumpKokiri, damage * 2);
    if (level > 0 && kokiriEffect != giantEffect) {
        SetEffect(table, kSlashKokiri, giantEffect);
        SetEffect(table, kSpinKokiri, giantEffect);
        SetEffect(table, kJumpKokiri, giantEffect);
    }
}

} // namespace

extern "C" int32_t OotmmExtraSwords_Level(void) {
    return Level();
}

extern "C" uint8_t OotmmExtraSwords_SwordDamage(int32_t dmgFlags, uint8_t damage) {
    if (!OotmmSession_IsActive()) {
        return damage;
    }
    const int kokiri = 1 + Level();
    if (dmgFlags & (kDmgSlashKokiri | kDmgSpinKokiri)) {
        return static_cast<uint8_t>(kokiri);
    }
    if (dmgFlags & kDmgJumpKokiri) {
        return static_cast<uint8_t>(kokiri * 2);
    }
    return damage;
}

extern "C" float OotmmExtraSwords_KokiriLength(float length) {
    return Level() == 2 ? 4000.0f : length;
}

extern "C" int32_t OotmmExtraSwords_KokiriSpinDamage(int32_t great) {
    const int32_t damage = 2 + Level();
    return great ? damage * 2 : damage;
}

extern "C" const char* OotmmExtraSwords_Icon(uint8_t item) {
    if (item != ITEM_SWORD_KOKIRI || Level() == 0) {
        return nullptr;
    }
    return kIcons[Level() - 1];
}

extern "C" Gfx* OotmmExtraSwords_Hand(PlayState* play, Gfx* closedHand) {
    if (!KokiriUpgraded()) {
        return nullptr;
    }
    if (Level() == 1) {
        return OotmmCustomItems_EquipmentDList(play, closedHand, kRazorHand, 1);
    }
    return OotmmCustomItems_EquipmentDList(play, closedHand, kGildedHand, 2);
}

extern "C" Gfx* OotmmExtraSwords_Sheath(PlayState* play, int32_t shield, int32_t shieldOnBack, int32_t swordInHand) {
    if (!LINK_IS_CHILD || !KokiriUpgraded()) {
        return nullptr;
    }
    const char* parts[2];
    int count = 0;
    if (shieldOnBack) {
        switch (shield) {
            case PLAYER_SHIELD_DEKU:
                parts[count++] = dgLinkChildDekuShieldWithMatrixDL;
                break;
            case PLAYER_SHIELD_HYLIAN:
                parts[count++] = "__OTR__objects/mm_obj_link_child/gLinkHumanHerosShieldWithMtxDL";
                break;
            case PLAYER_SHIELD_MIRROR:
                parts[count++] = "__OTR__objects/mm_obj_link_child/gLinkHumanMirrorShieldWithMtxDL";
                break;
            default:
                break;
        }
    }
    parts[count++] = (swordInHand ? kSheathEmpty : kSheathed)[Level() - 1];
    return OotmmCustomItems_EquipmentDList(play, nullptr, parts, count);
}

extern "C" void OotmmExtraSwords_Init(void) {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::ShouldActorUpdate>(UpdateDamageTable);
}
