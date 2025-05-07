#include "QueryManager.h"
#include <iostream>
#include <string>

const std::string titleText = "   ___             ____   ___  _     \r\n  / _ \\ _   _ _ __/ ___| / _ \\| |    \r\n | | | | | | | \'__\\___ \\| | | | |    \r\n | |_| | |_| | |   ___) | |_| | |___ \r\n  \\___/ \\__,_|_|  |____/ \\__\\_\\_____|\r\n";

int main() {
    const std::string dbPath = "."; 
    OurSQL::QueryManager qm(dbPath);

    std::cout << titleText << std::endl;
    std::string line;
    std::cout << "OurSQL > ";
    while (std::getline(std::cin, line)) {
        if (line == "exit" || line == "quit") break;
        qm.handleQuery(line);
        std::cout << "OurSQL > ";
    }
    return 0;
}
