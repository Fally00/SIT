#pragma once
#include "formatter/formatter.h"
#include <string>
#include <vector>

class CLI {
public:
    CLI(int argc, char* argv[]);
    void run();
private:
    std::vector<std::string> args;
    bool interactive = true;
    OutputFormat format = OutputFormat::Terminal;
    bool watchMode = false;
    int  watchIntervalMs = 1000;

    void showHelp();
    void showVersion();
    void showInfo();
    void showUsage();
    void showHealth();
    void showScan();
    void showAll();
    void showSchema();
    void showIntegrity(const std::vector<std::string>& tokens);
    void runWatch(const std::string& command);
    void interactiveMode();
    static std::vector<std::string> tokenize(const std::string& line);
};
