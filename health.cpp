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

// Implementation of System Health functions
namespace Health {

    // Returns the CPU health score
    std::string CPUhp(double cpuUsage) {
        if (cpuUsage < 50.0) {
            return "Good";
        } else if (cpuUsage < 80.0) {
            return "Moderate";
        } else {
            return "critical";
        }
    }

    // Returns the RAM health score
    std::string RAMhp(double ramUsage) {
        if (ramUsage < 50.0) {
            return "Good";
        } else if (ramUsage < 80.0) {
            return "Moderate";
        } else {
            return "critical";
        }
    }

    // Returns the disk health score
    int Diskhp(double diskUsage) {
        if (diskUsage < 70.0) {
            return 100;
        } else if (diskUsage < 90.0) {
            return 70;
        } else {
            return 40;
        }
    }

    // Returns overall health score
    int overallScore(double cpu, double ram, double disk) {
        int cpuScore = (cpu < 50.0) ? 100 : (cpu < 80.0) ? 70 : 40;
        int ramScore = (ram < 50.0) ? 100 : (ram < 80.0) ? 70 : 40;
        int diskScore = (disk < 70.0) ? 100 : (disk < 90.0) ? 70 : 40;
        return (cpuScore + ramScore + diskScore) / 3;
    }

}