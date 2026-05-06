#include "cli/cli.h"
#include "core/systemInfo.h"
#include "core/health.h"
#include "integrity/integrity.h"
#include "core/config.h"
#include "formatter/formatter.h"
#include "ui/colors.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <limits>
#include <thread>
#include <chrono>
#include <csignal>
#include <unordered_map>

// ── Signal handling for --watch mode ─────────────────────────────
static volatile std::sig_atomic_t g_running = 1;
static void signalHandler(int) { g_running = 0; }

namespace {
    bool isWatchableCommand(const std::string& cmd) {
        return cmd == "info" || cmd == "usage" || cmd == "health" || cmd == "scan" || cmd == "all";
    }

    std::string trimWhitespace(const std::string& input) {
        const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        });
        if (first == input.end()) {
            return {};
        }

        const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
            return std::isspace(ch) != 0;
        }).base();
        return std::string(first, last);
    }

    std::string collapseWhitespace(const std::string& input) {
        std::string normalized;
        normalized.reserve(input.size());

        bool inWhitespace = false;
        for (unsigned char ch : input) {
            if (std::isspace(ch) != 0) {
                if (!inWhitespace) {
                    normalized.push_back(' ');
                    inWhitespace = true;
                }
                continue;
            }

            normalized.push_back(static_cast<char>(ch));
            inWhitespace = false;
        }

        return normalized;
    }

    std::string normalizeInteractiveInput(const std::string& input) {
        return collapseWhitespace(trimWhitespace(input));
    }

    std::string toLowerCopy(const std::string& input) {
        std::string lowered = input;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return lowered;
    }

    const std::vector<std::string>& interactiveCommandNames() {
        static const std::vector<std::string> commands = {
            "help", "version", "info", "usage", "health",
            "scan", "all", "schema", "integrity", "exit", "quit"
        };
        return commands;
    }

    size_t levenshteinDistance(const std::string& lhs, const std::string& rhs) {
        std::vector<size_t> previous(rhs.size() + 1);
        std::vector<size_t> current(rhs.size() + 1);

        for (size_t j = 0; j <= rhs.size(); ++j) {
            previous[j] = j;
        }

        for (size_t i = 0; i < lhs.size(); ++i) {
            current[0] = i + 1;
            for (size_t j = 0; j < rhs.size(); ++j) {
                const size_t insertion = current[j] + 1;
                const size_t deletion = previous[j + 1] + 1;
                const size_t substitution = previous[j] + (lhs[i] == rhs[j] ? 0 : 1);
                current[j + 1] = std::min({ insertion, deletion, substitution });
            }
            previous.swap(current);
        }

        return previous[rhs.size()];
    }

    std::string findClosestCommand(const std::string& input) {
        if (input.empty()) {
            return {};
        }

        for (const auto& command : interactiveCommandNames()) {
            if (command.rfind(input, 0) == 0 || input.rfind(command, 0) == 0) {
                return command;
            }
        }

        size_t bestDistance = std::numeric_limits<size_t>::max();
        std::string bestMatch;
        for (const auto& command : interactiveCommandNames()) {
            size_t distance = levenshteinDistance(input, command);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestMatch = command;
            }
        }

        if (bestDistance <= 3) {
            return bestMatch;
        }
        return {};
    }
}

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
int CLI::run() {
    if (watchMode && args.empty()) {
        std::cerr << Color::Red
                  << "Error: --watch requires a command (info, usage, health, scan, or all)."
                  << Color::Reset << std::endl;
        return 1;
    }

    if (interactive) {
        interactiveMode();
        return 0;
    }

    const std::string& cmd = args[0];

    if (watchMode) {
        if (!isWatchableCommand(cmd)) {
            std::cerr << Color::Red
                      << "Error: --watch supports info, usage, health, scan, and all."
                      << Color::Reset << std::endl;
            return 1;
        }
        runWatch(cmd);
        return 0;
    }

    if      (cmd == "--help" || cmd == "-h" || cmd == "help")       showHelp();
    else if (cmd == "--version" || cmd == "-v" || cmd == "version") showVersion();
    else if (cmd == "info")      showInfo();
    else if (cmd == "usage")     showUsage();
    else if (cmd == "health")    showHealth();
    else if (cmd == "scan")      showScan();
    else if (cmd == "all")       showAll();
    else if (cmd == "schema")    showSchema();
    else if (cmd == "integrity") showIntegrity(args);
    else {
        std::cout << Color::Red << "Unknown command. Use --help for usage." << Color::Reset << std::endl;
        return 1;
    }
    return 0;
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
              << Color::White  << "  --watch                     Continuous monitoring for info|usage|health|scan|all (Ctrl+C to stop)\n"
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
bool CLI::dispatchInteractiveCommand(const std::vector<std::string>& tokens, const std::string& input) {
    using InteractiveHandler = bool (CLI::*)(const std::vector<std::string>&);
    static const std::unordered_map<std::string, InteractiveHandler> kCommandTable = {
        { "exit",      &CLI::handleInteractiveExit },
        { "quit",      &CLI::handleInteractiveExit },
        { "help",      &CLI::handleInteractiveHelp },
        { "version",   &CLI::handleInteractiveVersion },
        { "info",      &CLI::handleInteractiveInfo },
        { "usage",     &CLI::handleInteractiveUsage },
        { "health",    &CLI::handleInteractiveHealth },
        { "scan",      &CLI::handleInteractiveScan },
        { "all",       &CLI::handleInteractiveAll },
        { "schema",    &CLI::handleInteractiveSchema },
        { "integrity", &CLI::handleInteractiveIntegrity },
    };

    auto found = kCommandTable.find(tokens[0]);
    if (found != kCommandTable.end()) {
        return (this->*found->second)(tokens);
    }

    std::cout << Color::Red
              << "Unknown command: '" << input << "'. Type 'help' for available commands."
              << Color::Reset << std::endl;

    std::string suggestion = findClosestCommand(tokens[0]);
    if (!suggestion.empty()) {
        std::cout << Color::Yellow << "did you mean: " << suggestion << "?" << Color::Reset << std::endl;
    }

    return true;
}

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
bool CLI::handleInteractiveExit(const std::vector<std::string>&) {
    std::cout << Color::Red << "Exiting " << Config::kAppName << "." << Color::Reset << std::endl;
    return false;
}

bool CLI::handleInteractiveHelp(const std::vector<std::string>&) {
    showHelp();
    return true;
}

bool CLI::handleInteractiveVersion(const std::vector<std::string>&) {
    showVersion();
    return true;
}

bool CLI::handleInteractiveInfo(const std::vector<std::string>&) {
    showInfo();
    return true;
}

bool CLI::handleInteractiveUsage(const std::vector<std::string>&) {
    showUsage();
    return true;
}

bool CLI::handleInteractiveHealth(const std::vector<std::string>&) {
    showHealth();
    return true;
}

bool CLI::handleInteractiveScan(const std::vector<std::string>&) {
    showScan();
    return true;
}

bool CLI::handleInteractiveAll(const std::vector<std::string>&) {
    showAll();
    return true;
}

bool CLI::handleInteractiveSchema(const std::vector<std::string>&) {
    showSchema();
    return true;
}

bool CLI::handleInteractiveIntegrity(const std::vector<std::string>& tokens) {
    showIntegrity(tokens);
    return true;
}

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
        std::string normalizedInput = normalizeInteractiveInput(command);
        std::vector<std::string> tokens = tokenize(normalizedInput);
        if (tokens.empty()) continue;

        tokens[0] = toLowerCopy(tokens[0]);
        if (tokens[0] == "integrity" && tokens.size() > 1) {
            tokens[1] = toLowerCopy(tokens[1]);
        }

        if (!dispatchInteractiveCommand(tokens, normalizedInput)) {
            break;
        }
    }
}
