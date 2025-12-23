#pragma once 
#include <string>
#include <cstdint>

namespace SystemInfo {
    // Returns the operating system name
    std::string getOSName();

    //Returns the CPU model name , CPU usage percentage & CPU health score
    std::string getCPUModel();
    double getCPUusage();

    // Returns the ram size in GB , RAM usage percentage & RAM health score
    std::string getRam();
    double getRamUsage();
    // Returns the total physical disk size in GB & disk health score
    uint64_t getDisk();
    double getDiskUsage();
    // Returns the uptime in seconds
    int getUptime();

    //Returns the current user name
    std::string getUserName();
} 