#include "formatter/formatter.h"
#include "core/health.h"
#include "core/config.h"

#include <sstream>
#include <iomanip>
#include <ctime>

// ── ANSI color constants ─────────────────────────────────────────
namespace Color {
    constexpr const char* Reset  = "\033[0m";
    constexpr const char* Bold   = "\033[1m";
    constexpr const char* Red    = "\033[31m";
    constexpr const char* Green  = "\033[32m";
    constexpr const char* Yellow = "\033[33m";
    constexpr const char* White  = "\033[37m";
    constexpr const char* Coffee = "\033[38;5;94m";
}

static const char* kSep = "==============================================";

// ── JSON helper ──────────────────────────────────────────────────
static std::string jsonEsc(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:   o += c;
        }
    }
    return o;
}

// ── Scanner ──────────────────────────────────────────────────────
namespace Scanner {
    std::vector<ScanIssue> scan(const SystemSnapshot& snap) {
        std::vector<ScanIssue> issues;

        if (snap.cpuUsagePercent >= Config::Scan::kCpuCritical)
            issues.push_back({"CRITICAL", "High CPU usage (" + std::to_string(static_cast<int>(snap.cpuUsagePercent)) + "%)", "Close heavy apps or check background processes."});
        else if (snap.cpuUsagePercent >= Config::Scan::kCpuWarning)
            issues.push_back({"WARNING", "Elevated CPU usage (" + std::to_string(static_cast<int>(snap.cpuUsagePercent)) + "%)", "Check for apps using unusual CPU."});

        if (snap.ramUsagePercent >= Config::Scan::kRamCritical)
            issues.push_back({"CRITICAL", "High RAM usage (" + std::to_string(static_cast<int>(snap.ramUsagePercent)) + "%)", "Close apps or upgrade memory if needed."});
        else if (snap.ramUsagePercent >= Config::Scan::kRamWarning)
            issues.push_back({"WARNING", "Elevated RAM usage (" + std::to_string(static_cast<int>(snap.ramUsagePercent)) + "%)", "Close unused apps and browser tabs."});

        int days = static_cast<int>(snap.uptimeSeconds / 86400);
        if (days >= Config::Scan::kUptimeCriticalDays)
            issues.push_back({"CRITICAL", "Long uptime (" + std::to_string(days) + " days)", "Reboot to clear leaks and apply updates."});
        else if (days >= Config::Scan::kUptimeWarningDays)
            issues.push_back({"WARNING", "Long uptime (" + std::to_string(days) + " days)", "Consider a reboot if issues appear."});

        if (snap.cpuTemperatureC.has_value()) {
            double t = snap.cpuTemperatureC.value();
            if (t >= Config::Scan::kTempCritical)
                issues.push_back({"CRITICAL", "High CPU temperature (" + std::to_string(static_cast<int>(t)) + " C)", "Check cooling, fans, and airflow."});
            else if (t >= Config::Scan::kTempWarning)
                issues.push_back({"WARNING", "Elevated CPU temperature (" + std::to_string(static_cast<int>(t)) + " C)", "Clean dust and ensure good airflow."});
        }
        return issues;
    }
}

// ── Formatter helpers ────────────────────────────────────────────
namespace Formatter {

    std::string formatBytes(uint64_t bytes) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        if      (bytes >= 1024ULL*1024*1024*1024) o << bytes / (1024.0*1024*1024*1024) << " TB";
        else if (bytes >= 1024ULL*1024*1024)      o << bytes / (1024.0*1024*1024)      << " GB";
        else if (bytes >= 1024ULL*1024)            o << bytes / (1024.0*1024)           << " MB";
        else if (bytes >= 1024ULL)                 o << bytes / 1024.0                  << " KB";
        else                                       o << bytes << " B";
        return o.str();
    }

    std::string formatUptime(int64_t s) {
        int64_t d = s / 86400, h = (s % 86400) / 3600, m = (s % 3600) / 60, sec = s % 60;
        std::ostringstream o;
        if (d > 0) o << d << "d ";
        if (h > 0) o << h << "h ";
        if (m > 0) o << m << "m ";
        o << sec << "s";
        return o.str();
    }

    std::string toIso8601(std::chrono::system_clock::time_point tp) {
        auto tt = std::chrono::system_clock::to_time_t(tp);
        std::tm tm_val{};
#ifdef _WIN32
        localtime_s(&tm_val, &tt);
#else
        localtime_r(&tt, &tm_val);
#endif
        std::ostringstream o;
        o << std::put_time(&tm_val, "%Y-%m-%dT%H:%M:%S%z");
        return o.str();
    }

    // ── Terminal formatters ──────────────────────────────────────

    std::string terminalInfo(const SystemSnapshot& s) {
        std::ostringstream o;
        o << Color::Yellow << Color::Bold << "---------- System Basic Info ----------" << Color::Reset << "\n"
          << "Timestamp:  " << toIso8601(s.timestamp) << "\n"
          << "OS Name:    " << s.osName << "\n"
          << "CPU Model:  " << s.cpuModel << "\n"
          << "RAM:        " << formatBytes(s.totalRamBytes) << "\n"
          << "Disk Size:  " << formatBytes(s.totalDiskBytes) << "\n"
          << "Uptime:     " << formatUptime(s.uptimeSeconds) << "\n"
          << "User Name:  " << s.userName << "\n"
          << Color::Coffee << kSep << Color::Reset << "\n";
        return o.str();
    }

    std::string terminalUsage(const SystemSnapshot& s) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(1);
        o << Color::Yellow << Color::Bold << "---------- System Usage Info ----------" << Color::Reset << "\n"
          << "CPU Usage:  " << s.cpuUsagePercent << " %\n"
          << "RAM Usage:  " << s.ramUsagePercent << " %"
          << "  (" << formatBytes(s.totalRamBytes - s.availableRamBytes) << " / " << formatBytes(s.totalRamBytes) << ")\n"
          << "Disk Usage: " << s.diskUsagePercent << " %"
          << "  (" << formatBytes(s.totalDiskBytes - s.freeDiskBytes) << " / " << formatBytes(s.totalDiskBytes) << ")\n"
          << Color::Coffee << kSep << Color::Reset << "\n";
        return o.str();
    }

    std::string terminalHealth(const SystemSnapshot& s) {
        std::ostringstream o;
        o << Color::Yellow << Color::Bold << "---------- System Health Info ----------" << Color::Reset << "\n"
          << "CPU Health:  " << Health::getCpuHealthLabel(s.cpuUsagePercent) << "\n"
          << "RAM Health:  " << Health::getRamHealthLabel(s.ramUsagePercent) << "\n"
          << "Disk Health: " << Health::getDiskHealthLabel(s.diskUsagePercent) << "\n"
          << "Overall Health Score: " << Health::overallScore(s.cpuUsagePercent, s.ramUsagePercent, s.diskUsagePercent) << "/100\n"
          << Color::Coffee << kSep << Color::Reset << "\n";
        return o.str();
    }

    std::string terminalScan(const SystemSnapshot& s) {
        auto issues = Scanner::scan(s);
        std::ostringstream o;
        o << Color::Yellow << Color::Bold << "---------- System Problem Scan ----------" << Color::Reset << "\n";
        if (issues.empty()) {
            o << Color::Green << "No problems detected." << Color::Reset << "\n";
        } else {
            for (const auto& i : issues)
                o << Color::Red << "[" << i.severity << "] " << Color::Reset << i.message << " | Tip: " << i.tip << "\n";
        }
        o << Color::Coffee << kSep << Color::Reset << "\n";
        return o.str();
    }

    std::string terminalAll(const SystemSnapshot& s) {
        return terminalInfo(s) + terminalUsage(s) + terminalHealth(s) + terminalScan(s);
    }

    // ── JSON formatters ──────────────────────────────────────────

    std::string jsonInfo(const SystemSnapshot& s) {
        std::ostringstream o;
        o << "{\n"
          << "  \"timestamp\": \""       << jsonEsc(toIso8601(s.timestamp)) << "\",\n"
          << "  \"os_name\": \""         << jsonEsc(s.osName)   << "\",\n"
          << "  \"cpu_model\": \""       << jsonEsc(s.cpuModel) << "\",\n"
          << "  \"total_ram_bytes\": "   << s.totalRamBytes     << ",\n"
          << "  \"total_disk_bytes\": "  << s.totalDiskBytes    << ",\n"
          << "  \"uptime_seconds\": "    << s.uptimeSeconds     << ",\n"
          << "  \"user_name\": \""       << jsonEsc(s.userName) << "\"\n"
          << "}";
        return o.str();
    }

    std::string jsonUsage(const SystemSnapshot& s) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        o << "{\n"
          << "  \"timestamp\": \""         << jsonEsc(toIso8601(s.timestamp)) << "\",\n"
          << "  \"cpu_usage_percent\": "   << s.cpuUsagePercent   << ",\n"
          << "  \"ram_usage_percent\": "   << s.ramUsagePercent   << ",\n"
          << "  \"ram_used_bytes\": "      << (s.totalRamBytes - s.availableRamBytes) << ",\n"
          << "  \"ram_total_bytes\": "     << s.totalRamBytes     << ",\n"
          << "  \"disk_usage_percent\": "  << s.diskUsagePercent  << ",\n"
          << "  \"disk_used_bytes\": "     << (s.totalDiskBytes - s.freeDiskBytes) << ",\n"
          << "  \"disk_total_bytes\": "    << s.totalDiskBytes    << "\n"
          << "}";
        return o.str();
    }

    std::string jsonHealth(const SystemSnapshot& s) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        o << "{\n"
          << "  \"timestamp\": \""         << jsonEsc(toIso8601(s.timestamp)) << "\",\n"
          << "  \"cpu_health\": \""        << Health::getCpuHealthLabel(s.cpuUsagePercent)  << "\",\n"
          << "  \"ram_health\": \""        << Health::getRamHealthLabel(s.ramUsagePercent)  << "\",\n"
          << "  \"disk_health\": \""       << Health::getDiskHealthLabel(s.diskUsagePercent)<< "\",\n"
          << "  \"overall_score\": "       << Health::overallScore(s.cpuUsagePercent, s.ramUsagePercent, s.diskUsagePercent) << ",\n"
          << "  \"cpu_usage_percent\": "   << s.cpuUsagePercent  << ",\n"
          << "  \"ram_usage_percent\": "   << s.ramUsagePercent  << ",\n"
          << "  \"disk_usage_percent\": "  << s.diskUsagePercent << "\n"
          << "}";
        return o.str();
    }

    std::string jsonScan(const SystemSnapshot& s) {
        auto issues = Scanner::scan(s);
        std::ostringstream o;
        o << "{\n"
          << "  \"timestamp\": \"" << jsonEsc(toIso8601(s.timestamp)) << "\",\n"
          << "  \"issues_count\": " << issues.size() << ",\n"
          << "  \"issues\": [";
        for (size_t i = 0; i < issues.size(); ++i) {
            if (i > 0) o << ",";
            o << "\n    {\"severity\": \"" << jsonEsc(issues[i].severity)
              << "\", \"message\": \"" << jsonEsc(issues[i].message)
              << "\", \"tip\": \"" << jsonEsc(issues[i].tip) << "\"}";
        }
        if (!issues.empty()) o << "\n  ";
        o << "]\n}";
        return o.str();
    }

    std::string jsonAll(const SystemSnapshot& s) {
        auto issues = Scanner::scan(s);
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        o << "{\n"
          << "  \"timestamp\": \""          << jsonEsc(toIso8601(s.timestamp)) << "\",\n"
          << "  \"os_name\": \""            << jsonEsc(s.osName)   << "\",\n"
          << "  \"cpu_model\": \""          << jsonEsc(s.cpuModel) << "\",\n"
          << "  \"user_name\": \""          << jsonEsc(s.userName) << "\",\n"
          << "  \"total_ram_bytes\": "      << s.totalRamBytes      << ",\n"
          << "  \"available_ram_bytes\": "  << s.availableRamBytes  << ",\n"
          << "  \"ram_usage_percent\": "    << s.ramUsagePercent    << ",\n"
          << "  \"total_disk_bytes\": "     << s.totalDiskBytes     << ",\n"
          << "  \"free_disk_bytes\": "      << s.freeDiskBytes      << ",\n"
          << "  \"disk_usage_percent\": "   << s.diskUsagePercent   << ",\n"
          << "  \"cpu_usage_percent\": "    << s.cpuUsagePercent    << ",\n"
          << "  \"cpu_temperature_c\": "    << (s.cpuTemperatureC.has_value() ? std::to_string(s.cpuTemperatureC.value()) : "null") << ",\n"
          << "  \"uptime_seconds\": "       << s.uptimeSeconds      << ",\n"
          << "  \"cpu_health\": \""         << Health::getCpuHealthLabel(s.cpuUsagePercent)   << "\",\n"
          << "  \"ram_health\": \""         << Health::getRamHealthLabel(s.ramUsagePercent)   << "\",\n"
          << "  \"disk_health\": \""        << Health::getDiskHealthLabel(s.diskUsagePercent) << "\",\n"
          << "  \"overall_score\": "        << Health::overallScore(s.cpuUsagePercent, s.ramUsagePercent, s.diskUsagePercent) << ",\n"
          << "  \"issues_count\": "         << issues.size() << ",\n"
          << "  \"issues\": [";
        for (size_t i = 0; i < issues.size(); ++i) {
            if (i > 0) o << ",";
            o << "\n    {\"severity\": \"" << jsonEsc(issues[i].severity)
              << "\", \"message\": \"" << jsonEsc(issues[i].message)
              << "\", \"tip\": \"" << jsonEsc(issues[i].tip) << "\"}";
        }
        if (!issues.empty()) o << "\n  ";
        o << "]\n}";
        return o.str();
    }

    // ── CSV formatters ──────────────────────────────────────────

    std::string csvHeader() {
        return "timestamp,os_name,cpu_model,user_name,"
               "cpu_usage_percent,cpu_temperature_c,"
               "total_ram_bytes,available_ram_bytes,ram_usage_percent,"
               "total_disk_bytes,free_disk_bytes,disk_usage_percent,"
               "uptime_seconds,cpu_health,ram_health,disk_health,overall_score";
    }

    std::string csvRow(const SystemSnapshot& s) {
        auto esc = [](const std::string& v) -> std::string {
            if (v.find(',') == std::string::npos && v.find('"') == std::string::npos)
                return v;
            std::string o = "\"";
            for (char c : v) { if (c == '"') o += "\"\""; else o += c; }
            o += '"';
            return o;
        };
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        o << toIso8601(s.timestamp) << ","
          << esc(s.osName)  << ","
          << esc(s.cpuModel) << ","
          << esc(s.userName) << ","
          << s.cpuUsagePercent << ","
          << (s.cpuTemperatureC.has_value() ? std::to_string(s.cpuTemperatureC.value()) : "") << ","
          << s.totalRamBytes << ","
          << s.availableRamBytes << ","
          << s.ramUsagePercent << ","
          << s.totalDiskBytes << ","
          << s.freeDiskBytes << ","
          << s.diskUsagePercent << ","
          << s.uptimeSeconds << ","
          << Health::getCpuHealthLabel(s.cpuUsagePercent)  << ","
          << Health::getRamHealthLabel(s.ramUsagePercent)  << ","
          << Health::getDiskHealthLabel(s.diskUsagePercent) << ","
          << Health::overallScore(s.cpuUsagePercent, s.ramUsagePercent, s.diskUsagePercent);
        return o.str();
    }

    // ── Schema metadata ─────────────────────────────────────────

    std::string schemaInfo() {
        return R"({
  "schema_version": "1.0",
  "fields": [
    {"name": "timestamp",            "type": "string",  "unit": "ISO8601",  "description": "Measurement timestamp"},
    {"name": "os_name",              "type": "string",  "unit": null,       "description": "Operating system name"},
    {"name": "cpu_model",            "type": "string",  "unit": null,       "description": "CPU model identifier"},
    {"name": "user_name",            "type": "string",  "unit": null,       "description": "Current logged-in user"},
    {"name": "cpu_usage_percent",    "type": "float64", "unit": "percent",  "range": [0, 100], "description": "CPU utilization"},
    {"name": "cpu_temperature_c",    "type": "float64", "unit": "celsius",  "range": [0, 120], "description": "CPU temperature (null if unavailable)"},
    {"name": "total_ram_bytes",      "type": "uint64",  "unit": "bytes",    "description": "Total physical RAM"},
    {"name": "available_ram_bytes",  "type": "uint64",  "unit": "bytes",    "description": "Available physical RAM"},
    {"name": "ram_usage_percent",    "type": "float64", "unit": "percent",  "range": [0, 100], "description": "RAM utilization"},
    {"name": "total_disk_bytes",     "type": "uint64",  "unit": "bytes",    "description": "Total disk capacity"},
    {"name": "free_disk_bytes",      "type": "uint64",  "unit": "bytes",    "description": "Free disk space"},
    {"name": "disk_usage_percent",   "type": "float64", "unit": "percent",  "range": [0, 100], "description": "Disk utilization"},
    {"name": "uptime_seconds",       "type": "int64",   "unit": "seconds",  "description": "System uptime"},
    {"name": "cpu_health",           "type": "string",  "unit": null,       "enum": ["Good","Moderate","Critical"], "description": "CPU health label"},
    {"name": "ram_health",           "type": "string",  "unit": null,       "enum": ["Good","Moderate","Critical"], "description": "RAM health label"},
    {"name": "disk_health",          "type": "string",  "unit": null,       "enum": ["Good","Moderate","Critical"], "description": "Disk health label"},
    {"name": "overall_score",        "type": "int",     "unit": null,       "range": [0, 100], "description": "Composite health score"}
  ]
})";
    }

}
