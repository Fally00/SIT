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
        char* username = getenv("USER");
        if (username) {
            return std::string(username);
        }
        return "Unknown";
#endif
    }

}