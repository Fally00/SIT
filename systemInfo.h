#pragma once 
#include <string>

namespace SystemInfo {
    // Returns the operating system name
    std::string getOSName();

    //Returns the CPU model name
    std::string getCPUModel();

    // Returns the ram size in GB
    std::string getRam();

    // Returns the total physical disk size in GB
    uint64_t getDisk();

    // Returns the uptime in seconds
    int getUptime();

    //Returns the current user name
    std::string getUserName();
} 