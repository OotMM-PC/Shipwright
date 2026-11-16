#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <string>
#include <nlohmann/json.hpp>

extern "C" {
#endif

int32_t OotmmItemApply_BombchuCapacity(void);
int16_t OotmmWallet_ScaleRupees(int16_t amount);
int32_t OotmmWallet_HasWallet(void);
int32_t OotmmWallet_Bottomless(void);
void OotmmItemApply_AddBombchuAmmo(int32_t amount);
int32_t OotmmItemApply_SelectBombDrop(int32_t bombDrop, int32_t bombchuDrop);
struct PlayState;
int32_t OotmmItemApply_TrapDamageTick(struct PlayState* play, int32_t kind);

int32_t OotmmItemApply_FoundMasterSword(void);

int32_t OotmmItemApply_TradeCycleNeighbors(uint8_t slot, uint8_t* outLeft, uint8_t* outRight);

int32_t OotmmItemApply_TradeSlotCandidates(uint8_t slot, uint8_t* outItems, int32_t capacity);

int32_t OotmmItemApply_OwnedTradeItems(uint8_t slot, uint8_t* outItems, int32_t capacity);

void OotmmItemApply_SetTradeItemOwned(uint8_t slot, uint8_t item, int32_t owned);

void OotmmItemApply_ConsumeTradeItem(uint8_t slot, uint8_t item);

#ifdef __cplusplus
}

void OotmmItemApply_Init();
void OotmmItemApply_RecordBossClear(int dungeonId);
bool OotmmItemApply_CaptureSnapshot(nlohmann::json& snapshot);
nlohmann::json OotmmItemApply_LegacySnapshot(int fileNum);
void OotmmItemApply_SetSaveSnapshot(int fileNum, const nlohmann::json& snapshot, const nlohmann::json& shared = nullptr);
bool OotmmItemApply_CaptureSharedSnapshot(nlohmann::json& snapshot);
void OotmmItemApply_Reconcile();
nlohmann::json OotmmItemApply_TrapsApplied();
bool OotmmItemApply_PvpHit(int kind, int damage, int16_t yaw);

void OotmmItemApply_ResetLedgerForNewSave();

uint32_t OotmmItemApply_LedgerGet(const std::string& key);
void OotmmItemApply_LedgerSet(const std::string& key, uint32_t value);
#endif
