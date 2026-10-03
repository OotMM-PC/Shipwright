#include "OotmmFaroresWind.h"

#include "OotmmSession.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"

extern PlayState* gPlayState;
}

namespace {

constexpr uint32_t kOwnerNone = 0;
constexpr uint32_t kOwnerOot = 1;
constexpr uint32_t kOwnerMm = 2;

FaroresWindData& SlotForKey(std::string_view key) {
    const int32_t age = key == "fwAdult" ? LINK_AGE_ADULT : LINK_AGE_CHILD;
    return age == gSaveContext.linkAge ? gSaveContext.fw : gSaveContext.ship.backupFW;
}

uint32_t Owner(const FaroresWindData& slot) {
    if (!slot.set) {
        return kOwnerNone;
    }
    return OotmmFaroresWind_IsCrossEntrance(slot.entranceIndex) ? kOwnerMm : kOwnerOot;
}

void SyncRespawnTop() {
    auto& top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    const auto& fw = gSaveContext.fw;
    if (!fw.set) {
        top.data = 0;
        top.pos.x = 0.0f;
        top.pos.y = 0.0f;
        top.pos.z = 0.0f;
        return;
    }
    top.data = 0x28;
    top.pos.x = fw.pos.x;
    top.pos.y = fw.pos.y;
    top.pos.z = fw.pos.z;
    top.yaw = fw.yaw;
    top.playerParams = fw.playerParams;
    top.entranceIndex = fw.entranceIndex;
    top.roomIndex = fw.roomIndex;
    top.tempSwchFlags = fw.tempSwchFlags;
    top.tempCollectFlags = fw.tempCollectFlags;
}

} // namespace

bool OotmmFaroresWind_IsSharedKey(std::string_view key) {
    return key == "fwChild" || key == "fwAdult";
}

uint32_t OotmmFaroresWind_ReadShared(std::string_view key) {
    return Owner(SlotForKey(key));
}

void OotmmFaroresWind_WriteShared(std::string_view key, uint32_t owner) {
    auto& slot = SlotForKey(key);
    if (owner == Owner(slot)) {
        return;
    }
    if (owner == kOwnerMm) {
        slot = {};
        slot.set = 1;
        slot.entranceIndex = OOTMM_ENTR_FW_CROSS;
    } else {
        slot.set = 0;
    }
    if (&slot == &gSaveContext.fw && gPlayState != nullptr) {
        SyncRespawnTop();
    }
}

bool OotmmFaroresWind_ApplyArrival() {
    if (!gSaveContext.fw.set || OotmmFaroresWind_IsCrossEntrance(gSaveContext.fw.entranceIndex)) {
        return false;
    }
    gSaveContext.entranceIndex = static_cast<int16_t>(gSaveContext.fw.entranceIndex);
    gSaveContext.respawnFlag = 3;
    SyncRespawnTop();
    return true;
}

extern "C" int32_t OotmmFaroresWind_PerAge(void) {
    return OotmmSession_IsActive();
}

extern "C" void OotmmFaroresWind_SwapAge(void) {
    const FaroresWindData current = gSaveContext.fw;
    gSaveContext.fw = gSaveContext.ship.backupFW;
    gSaveContext.ship.backupFW = current;
}

extern "C" int32_t OotmmFaroresWind_IsCrossEntrance(int32_t entranceIndex) {
    return static_cast<uint16_t>(entranceIndex) == OOTMM_ENTR_FW_CROSS ? 1 : 0;
}

extern "C" int32_t OotmmFaroresWind_BeginCrossWarp(void) {
    if (!OotmmSession_IsActive() || !gSaveContext.fw.set ||
        !OotmmFaroresWind_IsCrossEntrance(gSaveContext.fw.entranceIndex)) {
        return 0;
    }
    Ship::OotmmEntranceMapping mapping;
    mapping.FromGame = Ship::OotmmGame::Oot;
    mapping.From = "OoT Farore's Wind";
    mapping.ToGame = Ship::OotmmGame::Mm;
    mapping.To = "Farore's Wind";
    mapping.ToNativeId = OOTMM_ENTR_FW_CROSS;
    OotmmSession_BeginCrossGameTransition(mapping);
    return 1;
}
