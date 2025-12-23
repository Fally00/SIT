#pragma once
#include <string>

namespace Health {

    std::string CPUhp(double cpuUsage);
    std::string RAMhp(double ramUsage);
    int Diskhp(double diskUsage);

    int overallScore(double cpu, double ram, double disk);
    
}
    