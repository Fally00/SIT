#pragma once
#include <string>
#include <vector>

class CLI {
public:
    CLI(int argc, char* argv[]);
    void run();

private:
    std::vector<std::string> args;
    bool interactive = true;

    void showHelp();
    void showVersion();
    void showInfo();
    void showUsage();
    void showHealth();
    void showAll();
    void interactiveMode();
};
