#include "platform/platform.h"

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#include <cstring>
#else
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#endif
#include <iomanip>

#include <algorithm>
#include <set>

namespace Platform {

std::string getCpuModel() {
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
    if (!file.is_open()) return "Unknown";
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t colon = line.find(':');
            if (colon != std::string::npos && colon + 2 < line.size())
                return line.substr(colon + 2);
        }
    }
    return "Unknown";
#endif
}

std::string getUserName() {
#ifdef _WIN32
    char buf[256];
    DWORD sz = sizeof(buf);
    if (GetUserNameA(buf, &sz)) return std::string(buf);
    return "Unknown";
#else
    const char* u = getenv("USER");
    return u ? std::string(u) : "Unknown";
#endif
}

std::optional<CpuTimes> readCpuTimes() {
#ifdef _WIN32
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) return std::nullopt;
    ULARGE_INTEGER iU, kU, uU;
    iU.LowPart  = idle.dwLowDateTime;    iU.HighPart  = idle.dwHighDateTime;
    kU.LowPart  = kernel.dwLowDateTime;  kU.HighPart  = kernel.dwHighDateTime;
    uU.LowPart  = user.dwLowDateTime;    uU.HighPart  = user.dwHighDateTime;
    return CpuTimes{iU.QuadPart, kU.QuadPart + uU.QuadPart};
#else
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return std::nullopt;
    std::string line;
    if (!std::getline(file, line)) return std::nullopt;
    std::istringstream iss(line);
    std::string cpu;
    uint64_t u=0, ni=0, sy=0, id=0, io=0, ir=0, si=0, st=0, gu=0, gn=0;
    iss >> cpu >> u >> ni >> sy >> id >> io >> ir >> si >> st >> gu >> gn;
    return CpuTimes{id + io, u + ni + sy + id + io + ir + si + st + gu + gn};
#endif
}

std::optional<double> readCpuTemperatureC() {
#ifdef _WIN32
    return std::nullopt;
#else
    const char* paths[] = {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/class/thermal/thermal_zone2/temp",
    };
    for (const char* p : paths) {
        std::ifstream file(p);
        long milliC = 0;
        if (file >> milliC) return milliC / 1000.0;
    }
    return std::nullopt;
#endif
}

MemoryInfo getMemoryInfo() {
    MemoryInfo mi;
#ifdef _WIN32
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        mi.totalBytes     = static_cast<uint64_t>(ms.ullTotalPhys);
        mi.availableBytes = static_cast<uint64_t>(ms.ullAvailPhys);
    }
#else
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return mi;
    std::string line;
    bool gotTotal = false, gotAvail = false;
    while (std::getline(file, line) && !(gotTotal && gotAvail)) {
        std::istringstream iss(line);
        std::string key, unit;
        uint64_t val = 0;
        iss >> key >> val >> unit;
        if (iss.fail()) continue;
        if (key == "MemTotal:")     { mi.totalBytes     = val * 1024; gotTotal = true; }
        if (key == "MemAvailable:") { mi.availableBytes = val * 1024; gotAvail = true; }
    }
#endif
    return mi;
}

DiskInfo getPrimaryDiskInfo() {
    DiskInfo d;
#ifdef _WIN32
    d.mountPoint = "C:\\";
    ULARGE_INTEGER totalBytes, freeBytes;
    if (GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, &freeBytes)) {
        d.totalBytes = totalBytes.QuadPart;
        d.freeBytes  = freeBytes.QuadPart;
    }
#else
    d.mountPoint = "/";
    try {
        auto sp = std::filesystem::space("/");
        d.totalBytes = sp.capacity;
        d.freeBytes  = sp.free;
    } catch (...) {}
#endif
    return d;
}

std::vector<DiskInfo> getAllDisks() {
    std::vector<DiskInfo> disks;
#ifdef _WIN32
    DWORD drives = GetLogicalDrives();
    std::set<std::string> seenMountPoints;
    for (int i = 0; i < 26; ++i) {
        if (!(drives & (1u << i))) continue;
        char root[] = { static_cast<char>('A' + i), ':', '\\', '\0' };
        UINT type = GetDriveTypeA(root);
        if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) continue;
        std::string mountPoint = root;
        if (!seenMountPoints.insert(mountPoint).second) continue;
        DiskInfo d;
        d.mountPoint = mountPoint;
        ULARGE_INTEGER total, free_;
        if (GetDiskFreeSpaceExA(root, NULL, &total, &free_)) {
            d.totalBytes = total.QuadPart;
            d.freeBytes  = free_.QuadPart;
        }
        disks.push_back(d);
    }
#else
    std::ifstream mounts("/proc/mounts");
    if (mounts.is_open()) {
        std::set<std::string> seenMountPoints;
        std::string line;
        while (std::getline(mounts, line)) {
            std::istringstream iss(line);
            std::string dev, mp, fstype;
            iss >> dev >> mp >> fstype;
            if (dev.empty() || dev[0] != '/') continue;
            if (fstype == "tmpfs" || fstype == "devtmpfs" || fstype == "squashfs") continue;
            try {
                auto sp = std::filesystem::space(mp);
                if (sp.capacity == 0) continue;
                if (!seenMountPoints.insert(mp).second) continue;

                DiskInfo d;
                d.mountPoint = mp;
                d.totalBytes = sp.capacity;
                d.freeBytes  = sp.free;
                disks.push_back(d);
            } catch (...) {}
        }
    }
    if (disks.empty()) disks.push_back(getPrimaryDiskInfo());
#endif
    return disks;
}

int64_t getUptimeSeconds() {
#ifdef _WIN32
    return static_cast<int64_t>(GetTickCount64() / 1000);
#else
    std::ifstream file("/proc/uptime");
    if (!file.is_open()) return 0;
    double seconds = 0.0;
    if (!(file >> seconds)) return 0;
    return static_cast<int64_t>(seconds);
#endif

}
#if !defined(_WIN32)
    std::string getOsName() {
        std::ifstream file("/etc/os-release");
        if (!file.is_open()) return "Linux (unknown)";
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("PRETTY_NAME") != std::string::npos) {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string val = line.substr(eq + 1);
                    if (val.size() >= 2 && val.front() == '"' && val.back() == '"')
                        val = val.substr(1, val.size() - 2);
                    return val;
                }
            }
        }
        return "Linux";
    }
#endif

} // namespace Platform