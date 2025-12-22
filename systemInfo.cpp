#include "systemInfo.h"       // Header with function declarations

#ifdef _WIN32
#define UNICODE
#include <windows.h>          // For Windows API functions
#include <intrin.h>          // For __cpuid
#include <cstring>          // For memcpy
#endif

#include <cstdint>          // For uint64_t
#include <string>          // For std::string
#include <sstream>        // For std::ostringstream
#include <iomanip>       // For std::fixed and std::setprecision

#ifndef _WIN32
#include <fstream>         // For file input/output (reading /proc and /etc files)
#include <filesystem>     // For std::filesystem::space (replaces statvfs for disk info)
#include <cstdlib>       // For getenv (replaces getlogin/getpwuid for username)
#endif

// Implementation of SystemInfo functions
namespace SystemInfo {

    // Returns the operating system name
    std::string getOSName() {
#ifdef _WIN32
        OSVERSIONINFOA osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOA));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (GetVersionExA(&osvi)) {
            if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
                return "Windows 11";
            } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) {
                return "Windows 10";
            } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2) {
                return "Windows 8";
            } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) {
                return "Windows 7";
            } else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1) {
                return "Windows XP";
            }
            return "Windows " + std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion);
        }
        return "Unknown";
#else
    // Linux implementation reading /etc/os-release
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
#endif
    }

    // Returns the CPU model name
    std::string getCPUModel() {
#ifdef _WIN32
        int cpuInfo[4] = {-1};
        char brand[49] = {0};
        __cpuid(cpuInfo, 0x80000002);
        memcpy(brand, cpuInfo, sizeof(cpuInfo));
        __cpuid(cpuInfo, 0x80000003);
        memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
        __cpuid(cpuInfo, 0x80000004);
        memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
        return std::string(brand);

#else
        // Linux implementation reading /proc/cpuinfo
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
#endif
    }

    // Returns the ram size in GB
    std::string getRam() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        double ramGB = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << ramGB << " GB";
        return oss.str();
#else
        // Linux implementation reading /proc/meminfo
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
#endif
    }

    // Returns the total physical disk size in GB
    uint64_t getDisk() {
#ifdef _WIN32
        ULARGE_INTEGER totalBytes;
        if (GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, NULL)) {
            return totalBytes.QuadPart / (1024LL * 1024 * 1024);
        }
        return 0;
#else
        // Linux implementation using std::filesystem
        try {
            auto space = std::filesystem::space("/");
            return space.capacity / (1024 * 1024 * 1024);
        } catch (...) {
            return 0;
        }
#endif
    }

    // Returns the uptime in seconds
    int getUptime() {
#ifdef _WIN32
        return static_cast<int>(GetTickCount64() / 1000);
#else
        // Linux implementation reading /proc/uptime
        std::ifstream file("/proc/uptime");
        double uptime_seconds;
        file >> uptime_seconds;
        return static_cast<int>(uptime_seconds);
#endif
    }

    // Returns the current user name
    std::string getUserName() {
#ifdef _WIN32
        char buffer[256];
        DWORD size = sizeof(buffer);
        if (GetUserNameA(buffer, &size)) {
            return std::string(buffer);
        }
        return "Unknown"; 
#else
        // Linux implementation version using getenv
        char* username = getenv("USER");
        if (username) {
            return std::string(username);
        }
        return "Unknown";
#endif
    }

    // Returns the CPU usage percentage
    double getCPUusage() {
#ifdef _WIN32
        static ULARGE_INTEGER lastIdle = {0}, lastKernel = {0}, lastUser = {0};

        FILETIME idleTime, kernelTime, userTime;
        if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
            return 0.0;

        // Convert FILETIME to ULARGE_INTEGER
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart   = idleTime.dwLowDateTime;
        idle.HighPart  = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart= kernelTime.dwHighDateTime;
        user.LowPart   = userTime.dwLowDateTime;
        user.HighPart  = userTime.dwHighDateTime;
        // Calculate differences
        uint64_t idleDiff   = idle.QuadPart   - lastIdle.QuadPart;
        uint64_t kernelDiff = kernel.QuadPart - lastKernel.QuadPart;
        uint64_t userDiff   = user.QuadPart   - lastUser.QuadPart;
        // Update last times
        lastIdle = idle;
        lastKernel = kernel;
        lastUser = user;
        // Calculate CPU usage
        uint64_t total = kernelDiff + userDiff;
        if (total == 0) return 0.0;

        return (1.0 - (double)idleDiff / total) * 100.0;
#else
        // Linux implementation: read /proc/stat
        static uint64_t prev_idle = 0, prev_total = 0;
        std::ifstream file("/proc/stat");
        std::string line;
        // Read the first line
        if (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string cpu;
            uint64_t user, nice, system, idle, iowait, irq, softirq;
            iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
            uint64_t total = user + nice + system + idle + iowait + irq + softirq;
            uint64_t idle_diff = idle - prev_idle;
            uint64_t total_diff = total - prev_total;
            prev_idle = idle;
            prev_total = total;
            // Calculate CPU usage
            if (total_diff == 0) return 0.0;
            return (1.0 - (double)idle_diff / total_diff) * 100.0;
        }
        return 0.0;
#endif
    }

    // Returns the CPU health score
    std::string getCPUhp() {
#ifdef _WIN32
        double usage = getCPUusage();
        if (usage < 50.0) {
            return "Good";
        } else if (usage < 80.0) {
            return "Moderate";
        } else {
            return "Poor";
        }
    }

#else
    // Linux implementation
        double usage = getCPUusage();
        if (usage < 50.0) {
            return "Good";
        } else if (usage < 80.0) {
            return "Moderate";
        } else {
            return "Poor";
        }
    }

#endif


    // Returns the RAM usage percentage
    double getRamUsage() {
#ifdef _WIN32
        // Windows implementation
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            return (1.0 - (double)memInfo.ullAvailPhys / memInfo.ullTotalPhys) * 100.0;
        }
        return 0.0;
#else
        // Linux implementation version reading /proc/meminfo
        std::ifstream file("/proc/meminfo");
        std::string line;
        // Variables to hold total and available memory
        uint64_t total = 0, available = 0;
        while (std::getline(file, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;
                total = std::stoull(value);
            } else if (line.find("MemAvailable:") != std::string::npos) {
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;
                available = std::stoull(value);
            }
        }
        // Calculate and return RAM usage percentage
        if (total == 0) return 0.0;
        return (1.0 - (double)available / total) * 100.0;
#endif
    }

    // Returns the RAM health score
    std::string getRamhp() {
#ifdef _WIN32
        double usage = getRamUsage();
        if (usage < 50.0) {
            return "Good";
        } else if (usage < 80.0) {
            return "Moderate";
        } else {
            return "Poor";
        }
    }
#else
    // Linux implementation
        double usage = getRamUsage();
        if (usage < 50.0) {
            return "Good";
        } else if (usage < 80.0) {
            return "Moderate";
        } else {
            return "Poor";
        }
    }
#endif

    // Returns the disk usage percentage
    double getDiskUsage() {
#ifdef _WIN32
        // Windows implementation
        ULARGE_INTEGER totalBytes, freeBytes;
        if (GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, &freeBytes)) {
            uint64_t used = totalBytes.QuadPart - freeBytes.QuadPart;
            return (double)used / totalBytes.QuadPart * 100.0;
        }
        return 0.0;
#else
        // Linux implementation using std::filesystem
        try {
            auto space = std::filesystem::space("/");
            uint64_t used = space.capacity - space.free;
            return (double)used / space.capacity * 100.0;
        } catch (...) {
            return 0.0;
        }
#endif
    }

    // Returns the disk health score
    uint64_t getDiskhp() {
#ifdef _WIN32
        double usage = getDiskUsage();
        if (usage < 70.0) {
            return 100;
        } else if (usage < 90.0) {
            return 70;
        } else {
            return 40;
        }
    }
#else
    // Linux implementation
        double usage = getDiskUsage();
        if (usage < 70.0) {
            return 100;
        } else if (usage < 90.0) {
            return 70;
        } else {
            return 40;
        }
    }
#endif

}
