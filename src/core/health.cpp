#include "core/health.h"
#include "core/config.h"

namespace Health {

    std::string getCpuHealthLabel(double cpuUsage) {
        if (cpuUsage < Config::Health::kCpuGoodMax)     return "Good";
        if (cpuUsage < Config::Health::kCpuModerateMax) return "Moderate";
        return "Critical";
    }

    std::string getRamHealthLabel(double ramUsage) {
        if (ramUsage < Config::Health::kRamGoodMax)     return "Good";
        if (ramUsage < Config::Health::kRamModerateMax) return "Moderate";
        return "Critical";
    }

    std::string getDiskHealthLabel(double diskUsage) {
        if (diskUsage < Config::Health::kDiskGoodMax)     return "Good";
        if (diskUsage < Config::Health::kDiskModerateMax) return "Moderate";
        return "Critical";
    }

    int overallScore(double cpu, double ram, double disk) {
        auto score = [](double usage, double goodMax, double modMax) -> int {
            if (usage < goodMax) return 100;
            if (usage < modMax)  return 70;
            return 40;
        };
        int c = score(cpu,  Config::Health::kCpuGoodMax,  Config::Health::kCpuModerateMax);
        int r = score(ram,  Config::Health::kRamGoodMax,  Config::Health::kRamModerateMax);
        int d = score(disk, Config::Health::kDiskGoodMax, Config::Health::kDiskModerateMax);
        return (c + r + d) / 3;
    }

}
