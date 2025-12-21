#define UNICODE
#include "systemInfo.h"      // Header with function declarations
#include <windows.h>        // For Windows API functions
#include <string>          // For std::string
#include <cstdint>        // For uint64_t
#include <sstream>       // For std::ostringstream
#include <iomanip>      // For std::fixed and std::setprecision
#include <intrin.h>    // For __cpuid

// Implementation of SystemInfo functions for Windows
namespace SystemInfo {

    // Returns the operating system name
    std::string getOSName() {
        OSVERSIONINFOA osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOA));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        if (GetVersionExA(&osvi)) {
            if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0) {
                return "Windows 10";
            } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) {
                return "Windows 8.1";
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
    }

    // Returns the CPU model name
    std::string getCPUModel() {
        int cpuInfo[4] = {-1};
        char brand[49] = {0};
        __cpuid(cpuInfo, 0x80000002);
        memcpy(brand, cpuInfo, sizeof(cpuInfo));
        __cpuid(cpuInfo, 0x80000003);
        memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
        __cpuid(cpuInfo, 0x80000004);
        memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
        return std::string(brand);
    }

    // Returns the ram size in GB
    std::string getRam() {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        double ramGB = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << ramGB << " GB";
        return oss.str();
    }

    // Returns the total physical disk size in GB
    uint64_t getDisk() {
        ULARGE_INTEGER totalBytes;
        if (GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, NULL)) {
            return totalBytes.QuadPart / (1024LL * 1024 * 1024);
        }
        return 0;
    }

    // Returns the uptime in seconds
    int getUptime() {
        return static_cast<int>(GetTickCount64() / 1000);
    }

    // Returns the current user name
    std::string getUserName() {
        char buffer[256];
        DWORD size = sizeof(buffer);
        if (GetUserNameA(buffer, &size)) {
            return std::string(buffer);
        }
        return "Unknown";
    }

}
