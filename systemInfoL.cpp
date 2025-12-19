#include "systemInfo.h"  // Header with function declarations
#include <fstream>       // For file input/output (reading /proc and /etc files)
#include <sstream>       // For string stream parsing (in getRam)
#include <iostream>      // Standard I/O (not directly used but common)
#include <filesystem>    // For std::filesystem::space (replaces statvfs for disk info)
#include <iomanip>       // For std::fixed and std::setprecision (in getRam formatting)
#include <cstdlib>       // For getenv (replaces getlogin/getpwuid for username)


// Implementation of SystemInfo functions for Linux
namespace SystemInfo {

    // Returns the operating system name
    std::string getOSName() {
        std::ifstream file("/etc/os-release");
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("PRETTY_NAME") != std::string::npos) {
                size_t equal = line.find('=');
                if (equal != std::string::npos) {
                    std::string value = line.substr(equal + 1);
                    if (!value.empty() && value[0] == '"') {
                        value = value.substr(1, value.size() - 2);
                    }
                    return value;
                }
            }
        }
        return "Linux";
    }

    // Returns the CPU model name
    std::string getCPUModel() {
        std::ifstream file("/proc/cpuinfo");
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    return line.substr(colon + 2);
                }
            }
        }
        return "Unknown";
    }

    // Returns the ram size in GB
    std::string getRam() {
        std::ifstream file("/proc/meminfo");
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("MemTotal") != std::string::npos) {
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;
                long mem_kb = std::stol(value);
                double mem_gb = mem_kb / (1024.0 * 1024.0);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << mem_gb << " GB";
                return oss.str();
            }
        }
        return "Unknown";
    }

    // Returns the total physical disk size in GB
    uint64_t getDisk() {
        try {
            auto space = std::filesystem::space("/");
            return space.capacity / (1024 * 1024 * 1024);
        } catch (...) {
            return 0;
        }
    }

    // Returns the uptime in seconds
    int getUptime() {
        std::ifstream file("/proc/uptime");
        double uptime_seconds;
        file >> uptime_seconds;
        return static_cast<int>(uptime_seconds);
    }

    // Returns the current user name
    std::string getUserName() {
        char* username = getenv("USER");
        if (username) {
            return std::string(username);
        }
        return "Unknown";
    }

}
