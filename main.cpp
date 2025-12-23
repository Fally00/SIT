#include "cli.h"
#include <iostream>

int main(int argc, char* argv[]) {
    CLI cli(argc, argv);
    cli.run();
    return 0;
}
