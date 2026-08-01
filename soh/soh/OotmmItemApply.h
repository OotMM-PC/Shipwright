#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <string>

extern "C" {
#endif

/// OoTMM's sticky "found the Master Sword" state, not the equipment bit OoT can clear.
int32_t OotmmItemApply_FoundMasterSword(void);

/// Nonzero only in a session where the slot has at least two owned items.
int32_t OotmmItemApply_TradeCycleNeighbors(uint8_t slot, uint8_t* outLeft, uint8_t* outRight);

/// Every item the trade slot can hold, in cycle order. Returns the count written.
int32_t OotmmItemApply_TradeSlotCandidates(uint8_t slot, uint8_t* outItems, int32_t capacity);

/// The slot's merged ownership (native save plus launcher grants). Returns the count written.
int32_t OotmmItemApply_OwnedTradeItems(uint8_t slot, uint8_t* outItems, int32_t capacity);

/// Keeps the native slot occupant, the ownership ledger and launcher debug grants consistent.
void OotmmItemApply_SetTradeItemOwned(uint8_t slot, uint8_t item, int32_t owned);

#ifdef __cplusplus
}

void OotmmItemApply_Init();
void OotmmItemApply_Reconcile();

void OotmmItemApply_ResetLedgerForNewSave();

uint32_t OotmmItemApply_LedgerGet(const std::string& key);
/// Records a ledger value and persists it immediately.
void OotmmItemApply_LedgerSet(const std::string& key, uint32_t value);
#endif
