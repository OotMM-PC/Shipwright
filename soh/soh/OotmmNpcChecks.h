#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int OotmmNpcChecks_IsShuffled(const char* nativeId);
int OotmmNpcChecks_DrawRutoSapphire(struct Actor* actor, struct PlayState* play);
int OotmmNpcChecks_ScrubPrice(struct Actor* scrub, struct PlayState* play, int vanillaPrice);
int OotmmNpcChecks_ScrubRepeatGi(struct Actor* scrub);
int OotmmNpcChecks_ScrubCanBuy(struct Actor* scrub, int price);
int OotmmNpcChecks_MerchantRepeatGi(const char* nativeId, int vanillaGi);
int OotmmNpcChecks_MerchantCanBuy(const char* nativeId, int price);
enum OotmmMerchant { OOTMM_MERCHANT_MEDIGORON, OOTMM_MERCHANT_CARPET, OOTMM_MERCHANT_GRANNY, OOTMM_MERCHANT_TALON };
int OotmmNpcChecks_MerchantPrice(int merchant, int vanillaPrice);
int OotmmNpcChecks_IsCollected(const char* nativeId);
int OotmmNpcChecks_Collect(const char* nativeId);
int OotmmNpcChecks_Done(const char* nativeId, int vanillaDone);
int OotmmNpcChecks_DungeonBeaten(int blueWarpFlag, int vanillaOwned);
void OotmmNpcChecks_HatchEgg(struct PlayState* play, int adult);
int OotmmNpcChecks_TryCollectFreestanding(struct Actor* actor, struct PlayState* play, const char* nativeId,
                                        float xzRange, float yRange);
const char* OotmmNpcChecks_BowlingPrizeSite(int prizeIndex);
const char* OotmmNpcChecks_DekuTheaterPrizeSite(int getItemId);
int OotmmNpcChecks_DrawCheckItem(struct PlayState* play, const char* nativeId);
int OotmmNpcChecks_BlueWarpReady(struct PlayState* play);
void OotmmNpcChecks_DrawBlueWarpReward(struct Actor* actor, struct PlayState* play);
void OotmmNpcChecks_Init(void);

#ifdef __cplusplus
}

#include <libultraship/bridge/OotmmGameState.h>

#include <string>

struct OotmmRepeatItem {
    int Gi;
    const char* ItemId;
    const char* Name;
    bool ViaLedger = false;
};

std::string OotmmNpcChecks_PlacementName(const Ship::OotmmPlacement& placement);
const OotmmRepeatItem* OotmmNpcChecks_FindRepeat(const Ship::OotmmPlacement& placement);
int OotmmNpcChecks_RepeatItem(const OotmmRepeatItem& repeat);
void OotmmNpcChecks_GiveRepeat(const OotmmRepeatItem& repeat, bool present = false);
int OotmmNpcChecks_SaleResult(const Ship::OotmmPlacement& placement, bool heartFallback, int price);
#endif
