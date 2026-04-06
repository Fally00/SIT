#pragma once
#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace Platform {
    std::string getOsName();
    std::string getCpuModel();
    std::string getUserName();

    struct CpuTimes {
        uint64_t idle  = 0;
        uint64_t total = 0;
    };
    std::optional<CpuTimes> readCpuTimes();
    std::optional<double> readCpuTemperatureC();

    struct MemoryInfo {
        uint64_t totalBytes     = 0;
        uint64_t availableBytes = 0;
    };
    MemoryInfo getMemoryInfo();

    struct DiskInfo {
        std::string mountPoint;
        uint64_t    totalBytes = 0;
        uint64_t    freeBytes  = 0;
    };
    DiskInfo              getPrimaryDiskInfo();
    std::vector<DiskInfo> getAllDisks();

    int64_t getUptimeSeconds();
}
