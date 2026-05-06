#include "core/systemInfo.h"
#include "platform/platform.h"
#include "core/config.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace SystemInfo {
    namespace {
        class CpuUsageSampler {
        public:
            CpuUsageSampler()
                : worker(&CpuUsageSampler::run, this) {}

            ~CpuUsageSampler() {
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    running = false;
                }
                wakeup.notify_one();
                if (worker.joinable()) {
                    worker.join();
                }
            }

            double load() const {
                return usagePercent.load(std::memory_order_relaxed);
            }

        private:
            std::atomic<double> usagePercent{0.0};
            std::condition_variable wakeup;
            std::mutex mutex;
            bool running = true;
            std::thread worker;

            void run() {
                auto previous = Platform::readCpuTimes();
                std::unique_lock<std::mutex> lock(mutex);

                while (running) {
                    if (wakeup.wait_for(
                            lock,
                            std::chrono::milliseconds(Config::kCpuSampleIntervalMs),
                            [this] { return !running; })) {
                        break;
                    }

                    lock.unlock();
                    auto current = Platform::readCpuTimes();
                    if (previous && current &&
                        current->idle >= previous->idle &&
                        current->total >= previous->total) {
                        uint64_t idleDiff = current->idle - previous->idle;
                        uint64_t totalDiff = current->total - previous->total;
                        if (totalDiff > 0) {
                            usagePercent.store(
                                (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0,
                                std::memory_order_relaxed);
                        }
                    }

                    if (current) {
                        previous = current;
                    } else {
                        previous.reset();
                    }
                    lock.lock();
                }
            }
        };

        CpuUsageSampler& cpuUsageSampler() {
            static CpuUsageSampler sampler;
            return sampler;
        }
    }

    std::string getOSName()    { return Platform::getOsName(); }
    std::string getCPUModel()  { return Platform::getCpuModel(); }
    std::string getUserName()  { return Platform::getUserName(); }
    int64_t     getUptime()    { return Platform::getUptimeSeconds(); }

    std::optional<double> getCpuTemperature() {
        return Platform::readCpuTemperatureC();
    }

    double getCpuUsage() {
        return cpuUsageSampler().load();
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
