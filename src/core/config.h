#pragma once

namespace Config {
    // ── Application ─────────────────────────────────────────────
    constexpr const char* kAppName = "System Insight Toolkit";
    constexpr const char* kVersion = "1.3.0";

    // ── CPU sampling ────────────────────────────────────────────
    constexpr int kCpuSampleIntervalMs = 200;

    // ── Health-score thresholds ─────────────────────────────────
    namespace Health {
        constexpr double kCpuGoodMax     = 50.0;
        constexpr double kCpuModerateMax = 80.0;

        constexpr double kRamGoodMax     = 50.0;
        constexpr double kRamModerateMax = 80.0;

        constexpr double kDiskGoodMax     = 70.0;
        constexpr double kDiskModerateMax = 90.0;
    }

    // ── Problem-scan thresholds ─────────────────────────────────
    namespace Scan {
        constexpr double kCpuWarning  = 70.0;
        constexpr double kCpuCritical = 85.0;

        constexpr double kRamWarning  = 75.0;
        constexpr double kRamCritical = 85.0;

        constexpr double kTempWarning  = 75.0;
        constexpr double kTempCritical = 85.0;

        constexpr int kUptimeWarningDays  = 7;
        constexpr int kUptimeCriticalDays = 30;
    }
}
