#include "OotmmFishing.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>

extern "C" {
#include "variables.h"
#include "z64.h"
}

namespace {

constexpr uint16_t kParamAdult = 0x1000;
constexpr uint16_t kParamLoach = 0x2000;
constexpr uint8_t kWeightLoachBit = 0x80;

struct PondFish {
    const char* Id;
    uint16_t Param;
};

constexpr std::array<PondFish, OOTMM_POND_FISH_ITEM_MAX> kPondFish = { {
    { "OOT_FISHING_POND_CHILD_FISH_2LBS", 0x0002 },
    { "OOT_FISHING_POND_CHILD_FISH_3LBS", 0x0003 },
    { "OOT_FISHING_POND_CHILD_FISH_4LBS", 0x0004 },
    { "OOT_FISHING_POND_CHILD_FISH_5LBS", 0x0005 },
    { "OOT_FISHING_POND_CHILD_FISH_6LBS", 0x0006 },
    { "OOT_FISHING_POND_CHILD_FISH_7LBS", 0x0007 },
    { "OOT_FISHING_POND_CHILD_FISH_8LBS", 0x0008 },
    { "OOT_FISHING_POND_CHILD_FISH_9LBS", 0x0009 },
    { "OOT_FISHING_POND_CHILD_FISH_10LBS", 0x000a },
    { "OOT_FISHING_POND_CHILD_FISH_11LBS", 0x000b },
    { "OOT_FISHING_POND_CHILD_FISH_12LBS", 0x000c },
    { "OOT_FISHING_POND_CHILD_FISH_13LBS", 0x000d },
    { "OOT_FISHING_POND_CHILD_FISH_14LBS", 0x000e },
    { "OOT_FISHING_POND_ADULT_FISH_4LBS", 0x1004 },
    { "OOT_FISHING_POND_ADULT_FISH_5LBS", 0x1005 },
    { "OOT_FISHING_POND_ADULT_FISH_6LBS", 0x1006 },
    { "OOT_FISHING_POND_ADULT_FISH_7LBS", 0x1007 },
    { "OOT_FISHING_POND_ADULT_FISH_8LBS", 0x1008 },
    { "OOT_FISHING_POND_ADULT_FISH_9LBS", 0x1009 },
    { "OOT_FISHING_POND_ADULT_FISH_10LBS", 0x100a },
    { "OOT_FISHING_POND_ADULT_FISH_11LBS", 0x100b },
    { "OOT_FISHING_POND_ADULT_FISH_12LBS", 0x100c },
    { "OOT_FISHING_POND_ADULT_FISH_13LBS", 0x100d },
    { "OOT_FISHING_POND_ADULT_FISH_14LBS", 0x100e },
    { "OOT_FISHING_POND_ADULT_FISH_15LBS", 0x100f },
    { "OOT_FISHING_POND_ADULT_FISH_16LBS", 0x1010 },
    { "OOT_FISHING_POND_ADULT_FISH_17LBS", 0x1011 },
    { "OOT_FISHING_POND_ADULT_FISH_18LBS", 0x1012 },
    { "OOT_FISHING_POND_ADULT_FISH_19LBS", 0x1013 },
    { "OOT_FISHING_POND_ADULT_FISH_20LBS", 0x1014 },
    { "OOT_FISHING_POND_ADULT_FISH_21LBS", 0x1015 },
    { "OOT_FISHING_POND_ADULT_FISH_22LBS", 0x1016 },
    { "OOT_FISHING_POND_ADULT_FISH_23LBS", 0x1017 },
    { "OOT_FISHING_POND_ADULT_FISH_24LBS", 0x1018 },
    { "OOT_FISHING_POND_ADULT_FISH_25LBS", 0x1019 },
    { "OOT_FISHING_POND_CHILD_LOACH_14LBS", 0x200e },
    { "OOT_FISHING_POND_CHILD_LOACH_15LBS", 0x200f },
    { "OOT_FISHING_POND_CHILD_LOACH_16LBS", 0x2010 },
    { "OOT_FISHING_POND_CHILD_LOACH_17LBS", 0x2011 },
    { "OOT_FISHING_POND_CHILD_LOACH_18LBS", 0x2012 },
    { "OOT_FISHING_POND_CHILD_LOACH_19LBS", 0x2013 },
    { "OOT_FISHING_POND_ADULT_LOACH_29LBS", 0x301d },
    { "OOT_FISHING_POND_ADULT_LOACH_30LBS", 0x301e },
    { "OOT_FISHING_POND_ADULT_LOACH_31LBS", 0x301f },
    { "OOT_FISHING_POND_ADULT_LOACH_32LBS", 0x3020 },
    { "OOT_FISHING_POND_ADULT_LOACH_33LBS", 0x3021 },
    { "OOT_FISHING_POND_ADULT_LOACH_34LBS", 0x3022 },
    { "OOT_FISHING_POND_ADULT_LOACH_35LBS", 0x3023 },
    { "OOT_FISHING_POND_ADULT_LOACH_36LBS", 0x3024 },
} };

struct PondFishStack {
    uint8_t* Count;
    uint8_t* Weights;
};

bool sHeldFromStack = false;

bool ShuffleActive() {
    return OotmmSession_IsActive() != 0 &&
           OotmmSession_GetState().GetBoolSetting("pondFishShuffle", false);
}

PondFishStack CurrentStack() {
    ShipOotmmPondFishData& data = gSaveContext.ship.ootmmPondFish;
    PondFishStack stack = gSaveContext.linkAge == LINK_AGE_CHILD
                              ? PondFishStack{ &data.childCount, data.childWeights }
                              : PondFishStack{ &data.adultCount, data.adultWeights };
    if (*stack.Count > OOTMM_POND_FISH_STACK_MAX) {
        *stack.Count = OOTMM_POND_FISH_STACK_MAX;
    }
    return stack;
}

void Push(bool adult, uint8_t weight) {
    ShipOotmmPondFishData& data = gSaveContext.ship.ootmmPondFish;
    uint8_t& count = adult ? data.adultCount : data.childCount;
    uint8_t* weights = adult ? data.adultWeights : data.childWeights;
    if (count >= OOTMM_POND_FISH_STACK_MAX) {
        return;
    }
    weights[count] = weight;
    count++;
}

void Reconcile() {
    const Ship::OotmmInventory& inventory = OotmmIpc_GetInventory();
    ShipOotmmPondFishData& data = gSaveContext.ship.ootmmPondFish;
    for (size_t i = 0; i < kPondFish.size(); i++) {
        uint32_t owned = inventory.Count(kPondFish[i].Id);
        if (owned > OOTMM_POND_FISH_STACK_MAX) {
            owned = OOTMM_POND_FISH_STACK_MAX;
        }
        if (owned <= data.granted[i]) {
            continue;
        }
        const bool adult = (kPondFish[i].Param & kParamAdult) != 0;
        uint8_t weight = static_cast<uint8_t>(kPondFish[i].Param & 0xFF);
        if ((kPondFish[i].Param & kParamLoach) != 0) {
            weight = static_cast<uint8_t>(weight | kWeightLoachBit);
        }
        for (uint32_t granted = data.granted[i]; granted < owned; granted++) {
            Push(adult, weight);
        }
        data.granted[i] = static_cast<uint8_t>(owned);
    }
}

float LengthForWeight(uint8_t weight) {
    return std::sqrt((static_cast<float>(weight & 0x7F) - 0.5f) / 0.0036f) + 1.0f;
}

} // namespace

// TODO: the pond actors are still vanilla, so real and shuffled fish coexist and a released fish can
// be reused; replacing them as checks needs the check system.
extern "C" void OotmmFishing_PeekOnHandFish(float* onHandLength, uint8_t* onHandIsLoach) {
    if (onHandLength == nullptr || onHandIsLoach == nullptr || !ShuffleActive()) {
        return;
    }
    Reconcile();

    const PondFishStack stack = CurrentStack();
    if (*stack.Count == 0) {
        if (sHeldFromStack) {
            sHeldFromStack = false;
            *onHandLength = 0.0f;
            *onHandIsLoach = 0;
        }
        return;
    }
    if (!sHeldFromStack && *onHandLength != 0.0f) {
        return;
    }

    const uint8_t weight = stack.Weights[*stack.Count - 1];
    sHeldFromStack = true;
    *onHandLength = LengthForWeight(weight);
    *onHandIsLoach = (weight & kWeightLoachBit) != 0 ? 1 : 0;
}

extern "C" void OotmmFishing_TakeOnHandFish(float* onHandLength, uint8_t* onHandIsLoach) {
    if (!ShuffleActive() || !sHeldFromStack) {
        return;
    }
    sHeldFromStack = false;

    const PondFishStack stack = CurrentStack();
    if (*stack.Count == 0) {
        return;
    }
    (*stack.Count)--;

    const uint8_t weight = stack.Weights[*stack.Count];
    if (onHandLength != nullptr) {
        *onHandLength = LengthForWeight(weight);
    }
    if (onHandIsLoach != nullptr) {
        *onHandIsLoach = (weight & kWeightLoachBit) != 0 ? 1 : 0;
    }
}

extern "C" void OotmmFishing_DiscardOnHandFish(float* onHandLength, uint8_t* onHandIsLoach) {
    if (!ShuffleActive() || !sHeldFromStack) {
        return;
    }
    sHeldFromStack = false;

    const PondFishStack stack = CurrentStack();
    if (*stack.Count != 0) {
        (*stack.Count)--;
    }
    if (onHandLength != nullptr) {
        *onHandLength = 0.0f;
    }
    if (onHandIsLoach != nullptr) {
        *onHandIsLoach = 0;
    }
}

extern "C" void OotmmFishing_ReleaseOnHandFish(void) {
    sHeldFromStack = false;
}
