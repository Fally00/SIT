#include "cli/cli.h"
#include <iostream>

int main(int argc, char* argv[]) {
    CLI cli(argc, argv);
    return cli.run();
}
