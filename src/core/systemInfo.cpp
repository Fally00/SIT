#include "core/systemInfo.h"
#include "platform/platform.h"
#include "core/config.h"

#include <thread>
#include <chrono>

namespace SystemInfo {

    std::string getOSName()    { return Platform::getOsName(); }
    std::string getCPUModel()  { return Platform::getCpuModel(); }
    std::string getUserName()  { return Platform::getUserName(); }
    int64_t     getUptime()    { return Platform::getUptimeSeconds(); }

    std::optional<double> getCpuTemperature() {
        return Platform::readCpuTemperatureC();
    }

    double getCpuUsage() {
        auto t1 = Platform::readCpuTimes();
        if (!t1) return 0.0;
        std::this_thread::sleep_for(std::chrono::milliseconds(Config::kCpuSampleIntervalMs));
        auto t2 = Platform::readCpuTimes();
        if (!t2) return 0.0;

        uint64_t idleDiff  = t2->idle  - t1->idle;
        uint64_t totalDiff = t2->total - t1->total;
        if (totalDiff == 0) return 0.0;
        return (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0;
    }

    uint64_t getTotalRamBytes()     { return Platform::getMemoryInfo().totalBytes; }
    uint64_t getAvailableRamBytes() { return Platform::getMemoryInfo().availableBytes; }

    double getRamUsage() {
        auto m = Platform::getMemoryInfo();
        if (m.totalBytes == 0) return 0.0;
        return (1.0 - static_cast<double>(m.availableBytes) / m.totalBytes) * 100.0;
    }

    uint64_t getTotalDiskBytes() { return Platform::getPrimaryDiskInfo().totalBytes; }
    uint64_t getFreeDiskBytes()  { return Platform::getPrimaryDiskInfo().freeBytes; }

    double getDiskUsage() {
        auto d = Platform::getPrimaryDiskInfo();
        if (d.totalBytes == 0) return 0.0;
        return static_cast<double>(d.totalBytes - d.freeBytes) / d.totalBytes * 100.0;
    }

    SystemSnapshot collectSnapshot() {
        SystemSnapshot s;
        s.timestamp = std::chrono::system_clock::now();

        s.osName   = Platform::getOsName();
        s.cpuModel = Platform::getCpuModel();
        s.userName = Platform::getUserName();

        s.cpuUsagePercent = getCpuUsage();
        s.cpuTemperatureC = Platform::readCpuTemperatureC();

        auto mem = Platform::getMemoryInfo();
        s.totalRamBytes     = mem.totalBytes;
        s.availableRamBytes = mem.availableBytes;
        if (s.totalRamBytes > 0)
            s.ramUsagePercent = (1.0 - static_cast<double>(s.availableRamBytes) / s.totalRamBytes) * 100.0;

        auto disk = Platform::getPrimaryDiskInfo();
        s.totalDiskBytes = disk.totalBytes;
        s.freeDiskBytes  = disk.freeBytes;
        if (s.totalDiskBytes > 0) {
            uint64_t used = s.totalDiskBytes - s.freeDiskBytes;
            s.diskUsagePercent = static_cast<double>(used) / s.totalDiskBytes * 100.0;
        }

        s.uptimeSeconds = Platform::getUptimeSeconds();
        return s;
    }

}
