#include "systemInfo.h"
#include <iostream>
using namespace std;


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



int main() {
    cout << MAGENTA << "System Information Tool" << RESET << endl;
    cout << MAGENTA << "       To Start the System Info tool Press {1} & {0} to Exit" << RESET << endl;
    cout << WHITE << "==============================================" << RESET << endl;

    //System Info Tool Start
    int choice;
    cin >> choice;
    if (choice == 1) {
        cout << WHITE << "==============================================" << RESET << endl;
        cout << "OS Name: " << SystemInfo::getOSName() << endl;
        cout << "CPU Model: " << SystemInfo::getCPUModel() << endl;
        cout << "RAM: " << SystemInfo::getRam() << endl;
        cout << "Disk Size: " << SystemInfo::getDisk() << " GB" << endl;
        cout << "Uptime: " << SystemInfo::getUptime() << " seconds" << endl;
        cout << "User Name: " << SystemInfo::getUserName() << endl;
        cout << WHITE << "==============================================" << RESET << endl;
    }
    //Exit System Info Tool
    else if(choice == 0){
        cout << RED << "Exiting System Information Tool." << RESET << endl;
    }




    return 0;
}
