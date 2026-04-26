#pragma once
#include "core/systemInfo.h"
#include <string>
#include <vector>

struct ScanIssue {
    std::string severity;
    std::string message;
    std::string tip;
};

enum class OutputFormat { Terminal, Json, Csv };

namespace Scanner {
    std::vector<ScanIssue> scan(const SystemSnapshot& snap);
}

namespace Formatter {
    std::string formatBytes(uint64_t bytes);
    std::string formatUptime(int64_t seconds);
    std::string toIso8601(std::chrono::system_clock::time_point tp);

    std::string terminalInfo(const SystemSnapshot& snap);
    std::string terminalUsage(const SystemSnapshot& snap);
    std::string terminalHealth(const SystemSnapshot& snap);
    std::string terminalScan(const SystemSnapshot& snap);
    std::string terminalAll(const SystemSnapshot& snap);

    std::string jsonInfo(const SystemSnapshot& snap);
    std::string jsonUsage(const SystemSnapshot& snap);
    std::string jsonHealth(const SystemSnapshot& snap);
    std::string jsonScan(const SystemSnapshot& snap);
    std::string jsonAll(const SystemSnapshot& snap);

    std::string csvHeader();
    std::string csvRow(const SystemSnapshot& snap);

    std::string schemaInfo();
}
