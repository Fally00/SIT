#include "cli/cli.h"
#include "core/systemInfo.h"
#include "core/health.h"
#include "integrity/integrity.h"
#include "core/config.h"
#include "formatter/formatter.h"

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>

// ANSI helpers (terminal-mode only)
namespace Color {
    constexpr const char* Reset  = "\033[0m";
    constexpr const char* Bold   = "\033[1m";
    constexpr const char* Red    = "\033[31m";
    constexpr const char* Green  = "\033[32m";
    constexpr const char* Yellow = "\033[33m";
    constexpr const char* White  = "\033[37m";
    constexpr const char* Coffee = "\033[38;5;94m";
}

// ── Signal handling for --watch mode ─────────────────────────────
static volatile bool g_running = true;
static void signalHandler(int) { g_running = false; }

// ── Constructor ──────────────────────────────────────────────────
CLI::CLI(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--format=", 0) == 0) {
            std::string fmt = arg.substr(9);
            if      (fmt == "json") format = OutputFormat::Json;
            else if (fmt == "csv")  format = OutputFormat::Csv;
            continue;
        }
        if (arg == "--watch") { watchMode = true; continue; }
        if (arg.rfind("--interval=", 0) == 0) {
            try { watchIntervalMs = std::stoi(arg.substr(11)); }
            catch (...) { watchIntervalMs = 1000; }
            if (watchIntervalMs < 100) watchIntervalMs = 100;
            continue;
        }
        args.push_back(arg);
    }
    if (!args.empty()) interactive = false;
}

// ── Run ──────────────────────────────────────────────────────────
void CLI::run() {
    if (interactive) { interactiveMode(); return; }

    const std::string& cmd = args[0];

    if (watchMode) { runWatch(cmd); return; }

    if      (cmd == "--help" || cmd == "-h" || cmd == "help")       showHelp();
    else if (cmd == "--version" || cmd == "-v" || cmd == "version") showVersion();
    else if (cmd == "info")      showInfo();
    else if (cmd == "usage")     showUsage();
    else if (cmd == "health")    showHealth();
    else if (cmd == "scan")      showScan();
    else if (cmd == "all")       showAll();
    else if (cmd == "schema")    showSchema();
    else if (cmd == "integrity") showIntegrity(args);
    else std::cout << Color::Red << "Unknown command. Use --help for usage." << Color::Reset << std::endl;
}

// ── Help ─────────────────────────────────────────────────────────
void CLI::showHelp() {
    std::cout << Color::Yellow << Color::Bold << Config::kAppName << " Commands" << Color::Reset << "\n\n"
              << Color::Coffee << "Commands:\n"
              << Color::White  << "  info       Show basic system information\n"
              << Color::White  << "  usage      Show system resource usage\n"
              << Color::White  << "  health     Show system health status\n"
              << Color::White  << "  scan       Scan for common system problems\n"
              << Color::White  << "  all        Show all information\n"
              << Color::White  << "  schema     Print field metadata (JSON)\n"
              << Color::White  << "  integrity  init|check|update [path]  File integrity tools\n"
              << Color::Green  << "  help       Show this help message\n"
              << Color::Green  << "  version    Show version information\n"
              << Color::Red    << "  << exit >>    Exit the tool\n\n"
              << Color::Coffee << "Options:\n"
              << Color::White  << "  --format=json|csv|terminal  Output format (default: terminal)\n"
              << Color::White  << "  --watch                     Continuous monitoring (Ctrl+C to stop)\n"
              << Color::White  << "  --interval=N                Polling interval in ms (default: 1000)\n"
              << Color::Reset  << std::endl;
}

// ── Version ──────────────────────────────────────────────────────
void CLI::showVersion() {
    if (format == OutputFormat::Json)
        std::cout << "{\"name\": \"" << Config::kAppName << "\", \"version\": \"" << Config::kVersion << "\"}" << std::endl;
    else
        std::cout << Color::Yellow << Config::kAppName << " v" << Config::kVersion << Color::Reset << std::endl;
}

// ── Schema ───────────────────────────────────────────────────────
void CLI::showSchema() {
    std::cout << Formatter::schemaInfo() << std::endl;
}

// ── Snapshot output helper ───────────────────────────────────────
static void printSnapshot(const SystemSnapshot& snap, OutputFormat fmt,
                          const std::string& cmd) {
    if (fmt == OutputFormat::Json) {
        if      (cmd == "info")   std::cout << Formatter::jsonInfo(snap);
        else if (cmd == "usage")  std::cout << Formatter::jsonUsage(snap);
        else if (cmd == "health") std::cout << Formatter::jsonHealth(snap);
        else if (cmd == "scan")   std::cout << Formatter::jsonScan(snap);
        else                      std::cout << Formatter::jsonAll(snap);
        std::cout << std::endl;
    } else if (fmt == OutputFormat::Csv) {
        std::cout << Formatter::csvRow(snap) << std::endl;
    } else {
        if      (cmd == "info")   std::cout << Formatter::terminalInfo(snap);
        else if (cmd == "usage")  std::cout << Formatter::terminalUsage(snap);
        else if (cmd == "health") std::cout << Formatter::terminalHealth(snap);
        else if (cmd == "scan")   std::cout << Formatter::terminalScan(snap);
        else                      std::cout << Formatter::terminalAll(snap);
    }
}

// ── Info / Usage / Health / Scan / All ────────────────────────────
void CLI::showInfo()   { auto s = SystemInfo::collectSnapshot(); printSnapshot(s, format, "info"); }
void CLI::showUsage()  { auto s = SystemInfo::collectSnapshot(); printSnapshot(s, format, "usage"); }
void CLI::showHealth() { auto s = SystemInfo::collectSnapshot(); printSnapshot(s, format, "health"); }
void CLI::showScan()   { auto s = SystemInfo::collectSnapshot(); printSnapshot(s, format, "scan"); }
void CLI::showAll()    { auto s = SystemInfo::collectSnapshot(); printSnapshot(s, format, "all"); }

// ── Watch Mode ───────────────────────────────────────────────────
void CLI::runWatch(const std::string& cmd) {
    std::signal(SIGINT,  signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Print CSV header once if in CSV mode
    if (format == OutputFormat::Csv)
        std::cout << Formatter::csvHeader() << std::endl;

    while (g_running) {
        auto snap = SystemInfo::collectSnapshot();
        printSnapshot(snap, format, cmd);
        std::cout.flush();

        // Sleep in small chunks so we can respond to signals promptly
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(watchIntervalMs);
        while (g_running && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    if (format == OutputFormat::Terminal)
        std::cout << "\n" << Color::Yellow << "Watch stopped." << Color::Reset << std::endl;
}

// ── Integrity ────────────────────────────────────────────────────
void CLI::showIntegrity(const std::vector<std::string>& tokens) {
    std::cout << Color::Yellow << Color::Bold << "---------- File Integrity ----------" << Color::Reset << std::endl;
    if (tokens.size() < 2) {
        std::cout << Color::White << "Usage: integrity init|check|update [path]" << std::endl;
        std::cout << Color::Coffee << "==============================================" << Color::Reset << std::endl;
        return;
    }

    std::string action = tokens[1];
    std::filesystem::path root = std::filesystem::current_path();
    if (tokens.size() >= 3) root = tokens[2];
    root = std::filesystem::absolute(root);
    std::filesystem::path manifestPath = root / Integrity::kManifestFileName;

    std::string error;
    if (action == "init") {
        bool ok = Integrity::createManifest(root, &error);
        if (ok) {
            std::cout << Color::Green << "Integrity manifest created at: " << manifestPath.string() << Color::Reset << std::endl;
            if (!error.empty()) std::cout << Color::Yellow << "Warning: " << error << Color::Reset << std::endl;
        } else {
            std::cout << Color::Red << (error.empty() ? "Failed to create integrity manifest." : error) << Color::Reset << std::endl;
        }
    } else if (action == "update") {
        bool ok = Integrity::updateManifest(root, &error);
        if (ok) {
            std::cout << Color::Green << "Integrity manifest updated at: " << manifestPath.string() << Color::Reset << std::endl;
            if (!error.empty()) std::cout << Color::Yellow << "Warning: " << error << Color::Reset << std::endl;
        } else {
            std::cout << Color::Red << (error.empty() ? "Failed to update integrity manifest." : error) << Color::Reset << std::endl;
        }
    } else if (action == "check") {
        Integrity::CheckResult result;
        bool ok = Integrity::checkManifest(root, result, &error);
        if (!ok) {
            std::cout << Color::Red << (error.empty() ? "Failed to check integrity manifest." : error) << Color::Reset << std::endl;
            std::cout << Color::Coffee << "==============================================" << Color::Reset << std::endl;
            return;
        }
        std::cout << Color::White << "Root: " << root.string() << "\n"
                  << "Manifest: " << manifestPath.string() << "\n"
                  << "Tracked: " << result.total << "\n"
                  << "OK: " << result.ok
                  << " | Changed: " << result.changed
                  << " | Missing: " << result.missing
                  << " | New: " << result.added
                  << " | Errors: " << result.errors << std::endl;
        if (result.issues.empty()) {
            std::cout << Color::Green << "Integrity OK." << Color::Reset << std::endl;
        } else {
            for (const auto& issue : result.issues)
                std::cout << Color::Red << "[" << issue.status << "] " << Color::Reset << issue.path << " | " << issue.detail << std::endl;
        }
    } else {
        std::cout << Color::White << "Usage: integrity init | check | update [path]" << std::endl;
    }
    std::cout << Color::Coffee << "==============================================" << Color::Reset << std::endl;
}

// ── Tokenizer ────────────────────────────────────────────────────
std::vector<std::string> CLI::tokenize(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    for (std::string t; iss >> t;) tokens.push_back(t);
    return tokens;
}

// ── Interactive Mode ─────────────────────────────────────────────
void CLI::interactiveMode() {
    const std::string logo = R"(
   _____ __________
  / ___//  _/_  __/
  \__ \ / /  / /
 ___/ // /  / /
/____/___/ /_/
                    )";

    std::cout << Color::Yellow << Color::Bold << logo << Color::Reset << "\n"
              << Color::Coffee << "==============================================" << Color::Reset << "\n"
              << Color::Yellow << Color::Bold << Config::kAppName << " v" << Config::kVersion << Color::Reset << "\n"
              << "Type 'help' for commands or 'exit' to quit.\n"
              << Color::Coffee << "==============================================" << Color::Reset << "\n";

    std::string command;
    while (true) {
        std::cout << "sysinfo> ";
        std::getline(std::cin, command);
        if (!std::cin) {
            std::cout << Color::Red << "Input closed. Exiting " << Config::kAppName << "." << Color::Reset << std::endl;
            break;
        }
        std::vector<std::string> tokens = tokenize(command);
        if (tokens.empty()) continue;

        const std::string& cmd = tokens[0];
        if      (cmd == "exit" || cmd == "quit") { std::cout << Color::Red << "Exiting " << Config::kAppName << "." << Color::Reset << std::endl; break; }
        else if (cmd == "help")      showHelp();
        else if (cmd == "version")   showVersion();
        else if (cmd == "info")      showInfo();
        else if (cmd == "usage")     showUsage();
        else if (cmd == "health")    showHealth();
        else if (cmd == "scan")      showScan();
        else if (cmd == "schema")    showSchema();
        else if (cmd == "integrity") showIntegrity(tokens);
        else if (cmd == "all")       showAll();
        else std::cout << Color::Red << "Unknown command. Type 'help' for available commands." << Color::Reset << std::endl;
    }
}
