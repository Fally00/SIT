#include "systemInfo.h"
#include "health.h"
#include "cli.h"
#include <string>
#include <vector>
#include <iostream>

// ANSI color codes
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define PURPLE      "\033[95m"
#define ORANGE      "\033[38;5;208m"
#define COFFEE     "\033[38;5;94m"

CLI::CLI(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    if (!args.empty()) {
        interactive = false;
    }
}

void CLI::run() {
    if (interactive) {
        interactiveMode();
    } else {
        if (args[0] == "--help" || args[0] == "-h") {
            showHelp();
        } else if (args[0] == "--version" || args[0] == "-v") {
            showVersion();
        } else if (args[0] == "info") {
            showInfo();
        } else if (args[0] == "usage") {
            showUsage();
        } else if (args[0] == "health") {
            showHealth();
        } else if (args[0] == "all") {
            showAll();
        } else {
            std::cout << RED << "Unknown command. Use --help for usage." << RESET << std::endl;
        }
    }
}

void CLI::showHelp() {
    std::cout << YELLOW << BOLD << "System Insight Toolkit Commands" << RESET << std::endl;
    std::cout << WHITE << std::endl;
    std::cout << COFFEE << "Commands:" << std::endl;
    std::cout << WHITE << "  info     Show basic system information" << std::endl;
    std::cout << WHITE << "  usage    Show system resource usage" << std::endl;
    std::cout << WHITE << "  health   Show system health status" << std::endl;
    std::cout << WHITE << "  all      Show all information" << std::endl;
    std::cout << GREEN << "  help     Show this help message" << std::endl;
    std::cout << GREEN << "  version  Show version information" << std::endl;
    std::cout << RED << "  << exit >>    Exit the tool" << std::endl;
    std::cout << WHITE << std::endl;
}

void CLI::showVersion() {
    std::cout << YELLOW << "System Insight Toolkit v1.2.0" << std::endl;
}

void CLI::showInfo() {
    std::cout << YELLOW << BOLD << "---------- System Basic Info ----------" << RESET << std::endl;
    std::cout << "OS Name: " << SystemInfo::getOSName() << std::endl;
    std::cout << "CPU Model: " << SystemInfo::getCPUModel() << std::endl;
    std::cout << "RAM: " << SystemInfo::getRam() << std::endl;
    std::cout << "Disk Size: " << SystemInfo::getDisk() << " GB" << std::endl;
    std::cout << "Uptime: " << SystemInfo::getUptime() << " seconds" << std::endl;
    std::cout << "User Name: " << SystemInfo::getUserName() << std::endl;
    std::cout << COFFEE << "==============================================" << RESET << std::endl;
}

void CLI::showUsage() {
    std::cout << YELLOW << BOLD << "---------- System Usage Info ----------" << RESET << std::endl;
    double cpu = SystemInfo::getCPUusage();
    double ram = SystemInfo::getRamUsage();
    double disk = SystemInfo::getDiskUsage();
    std::cout << "CPU Usage: " << cpu << " %" << std::endl;
    std::cout << "RAM Usage: " << ram << " %" << std::endl;
    std::cout << "Disk Usage: " << disk << " %" << std::endl;
    std::cout << COFFEE << "==============================================" << RESET << std::endl;
}

void CLI::showHealth() {
    std::cout << YELLOW<< BOLD << "---------- System Health Info ----------" << RESET << std::endl;
    double cpu = SystemInfo::getCPUusage();
    double ram = SystemInfo::getRamUsage();
    double disk = SystemInfo::getDiskUsage();
    std::cout << "CPU Health: " << Health::CPUhp(cpu) << std::endl;
    std::cout << "RAM Health: " << Health::RAMhp(ram) << std::endl;
    std::cout << "Disk Health: " << Health::Diskhp(disk) << std::endl;
    std::cout << "Overall Health Score: " << Health::overallScore(cpu, ram, disk) << "/100" << std::endl;
    std::cout << COFFEE << "==============================================" << RESET << std::endl;
}

void CLI::showAll() {
    showInfo();
    showUsage();
    showHealth();
}

void CLI::interactiveMode() {
    const std::string logo = R"(
   _____ __________ 
  / ___//  _/_  __/
  \__ \ / /  / /   
 ___/ // /  / /    
/____/___/ /_/     
                    )";

    std::cout << YELLOW  << BOLD << logo << RESET << std::endl;
    std::cout << COFFEE << "==============================================" << RESET << std::endl;
    std::cout << YELLOW << BOLD << "System Insight Toolkit v1.2.0" << RESET << std::endl;
    std::cout << "Type 'help' for commands or 'exit' to quit." << std::endl;
    std::cout << COFFEE << "==============================================" << RESET << std::endl;

    std::string command;
    while (true) {
        std::cout << "sysinfo> ";
        std::getline(std::cin, command);
        if (command == "exit" || command == "quit") {
            std::cout << RED << "Exiting System Insight Toolkit." << RESET << std::endl;
            break;
        } else if (command == "help") {
            showHelp();
        } else if (command == "version") {
            showVersion();
        } else if (command == "info") {
            showInfo();
        } else if (command == "usage") {
            showUsage();
        } else if (command == "health") {
            showHealth();
        } else if (command == "all") {
            showAll();
        } else if (command.empty()) {
            continue;
        } else {
            std::cout << RED << "Unknown command. Type 'help' for available commands." << RESET << std::endl;
        }
    }
}
