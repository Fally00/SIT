#pragma once 
#include <string>
#include <cstdint>

namespace SystemInfo {
    // Returns the operating system name
    std::string getOSName();

    //Returns the CPU model name , CPU usage percentage & CPU health score
    std::string getCPUModel();
    double getCPUusage();
    std::string getCPUhp();

    // Returns the ram size in GB , RAM usage percentage & RAM health score
    std::string getRam();
    double getRamUsage();
    std::string getRamhp();
    // Returns the total physical disk size in GB & disk health score
    uint64_t getDisk();
    uint64_t getDiskhp();
    // Returns the uptime in seconds
    int getUptime();

    //Returns the current user name
    std::string getUserName();
} 