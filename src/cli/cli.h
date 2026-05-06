#pragma once
#include "formatter/formatter.h"
#include <string>
#include <vector>

class CLI {
public:
    CLI(int argc, char* argv[]);
    int run();
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
    bool dispatchInteractiveCommand(const std::vector<std::string>& tokens, const std::string& input);
    bool handleInteractiveExit(const std::vector<std::string>& tokens);
    bool handleInteractiveHelp(const std::vector<std::string>& tokens);
    bool handleInteractiveVersion(const std::vector<std::string>& tokens);
    bool handleInteractiveInfo(const std::vector<std::string>& tokens);
    bool handleInteractiveUsage(const std::vector<std::string>& tokens);
    bool handleInteractiveHealth(const std::vector<std::string>& tokens);
    bool handleInteractiveScan(const std::vector<std::string>& tokens);
    bool handleInteractiveAll(const std::vector<std::string>& tokens);
    bool handleInteractiveSchema(const std::vector<std::string>& tokens);
    bool handleInteractiveIntegrity(const std::vector<std::string>& tokens);
    void interactiveMode();
    static std::vector<std::string> tokenize(const std::string& line);
};
