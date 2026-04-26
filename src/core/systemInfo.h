#pragma once
#include <string>
#include <cstdint>
#include <optional>
#include <chrono>

// ── Unified snapshot of all system metrics ───────────────────────
struct SystemSnapshot {
    std::chrono::system_clock::time_point timestamp;

    // Static / identifying
    std::string osName;
    std::string cpuModel;
    std::string userName;

    // CPU
    double cpuUsagePercent = 0.0;
    std::optional<double> cpuTemperatureC;  // nullopt when unavailable

    // RAM (bytes for precision; percentage for convenience)
    uint64_t totalRamBytes     = 0;
    uint64_t availableRamBytes = 0;
    double   ramUsagePercent   = 0.0;

    // Disk (bytes for precision; percentage for convenience)
    uint64_t totalDiskBytes = 0;
    uint64_t freeDiskBytes  = 0;
    double   diskUsagePercent = 0.0;

    // Uptime
    int64_t uptimeSeconds = 0;
};

// ── Individual accessors (all return raw values) ─────────────────
namespace SystemInfo {
    std::string getOSName();
    std::string getCPUModel();
    std::string getUserName();

    double getCpuUsage();
    std::optional<double> getCpuTemperature();

    uint64_t getTotalRamBytes();
    uint64_t getAvailableRamBytes();
    double   getRamUsage();

    uint64_t getTotalDiskBytes();
    uint64_t getFreeDiskBytes();
    double   getDiskUsage();

    int64_t getUptime();

    SystemSnapshot collectSnapshot();
}
