#pragma once

#include <string>
#include <vector>

void OotmmCheckLedger_Record(const std::string& checkId);
bool OotmmCheckLedger_Contains(const std::string& checkId);
std::vector<std::string> OotmmCheckLedger_CaptureSnapshot();
void OotmmCheckLedger_Init();
