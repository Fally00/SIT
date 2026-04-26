#pragma once
#include <string>

namespace Health {
    std::string getCpuHealthLabel(double cpuUsage);
    std::string getRamHealthLabel(double ramUsage);
    std::string getDiskHealthLabel(double diskUsage);
    int overallScore(double cpu, double ram, double disk);
}
