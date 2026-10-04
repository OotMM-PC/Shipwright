#include "OotmmCheckLedger.h"

#include "OotmmIpc.h"
#include "OotmmSession.h"
#include "SaveManager.h"

#include <algorithm>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace {

std::unordered_set<std::string> sReported;
std::mutex sMutex;

void LoadSection() {
    std::vector<std::string> checks;
    SaveManager::Instance->LoadData("checks", checks);
    {
        std::lock_guard<std::mutex> lock(sMutex);
        sReported = { checks.begin(), checks.end() };
    }
    for (const auto& checkId : checks) {
        if (!OotmmIpc_IsCheckCompleted(checkId)) {
            OotmmIpc_SendCheckCollected(checkId);
        }
    }
}

void InitSection(bool) {
    std::lock_guard<std::mutex> lock(sMutex);
    sReported.clear();
}

} // namespace

void OotmmCheckLedger_Record(const std::string& checkId) {
    if (!checkId.empty()) {
        std::lock_guard<std::mutex> lock(sMutex);
        sReported.emplace(checkId);
    }
}

bool OotmmCheckLedger_Contains(const std::string& checkId) {
    std::lock_guard<std::mutex> lock(sMutex);
    return sReported.count(checkId) != 0;
}

std::vector<std::string> OotmmCheckLedger_CaptureSnapshot() {
    std::unordered_set<std::string> reported;
    {
        std::lock_guard<std::mutex> lock(sMutex);
        reported = sReported;
    }
    const auto& state = OotmmSession_GetState();
    for (const auto& placement : state.GetPlacements()) {
        if (placement.CheckGame == Ship::OotmmGame::Oot &&
            (placement.SourcePlayer == 0 || placement.SourcePlayer == state.GetPlayerId()) &&
            OotmmIpc_IsCheckCompleted(placement.CheckId)) {
            reported.emplace(placement.CheckId);
        }
    }
    std::vector<std::string> checks(reported.begin(), reported.end());
    std::sort(checks.begin(), checks.end());
    return checks;
}

void OotmmCheckLedger_Init() {
    SaveManager::Instance->AddInitFunction(InitSection);
    SaveManager::Instance->AddLoadFunction("ootmmChecks", 1, LoadSection);
}
