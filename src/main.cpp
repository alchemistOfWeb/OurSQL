#include "MetaManager.h"
#include "FileManager.h"
#include "RecordManager.h"
#include "QueryProcessor.h"
#include "DatabaseManager.h"
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>


const std::string titleText = "   ___             ____   ___  _     \r\n  / _ \\ _   _ _ __/ ___| / _ \\| |    \r\n | | | | | | | \'__\\___ \\| | | | |    \r\n | |_| | |_| | |   ___) | |_| | |___ \r\n  \\___/ \\__,_|_|  |____/ \\__\\_\\_____|\r\n";


int main() {
    // std::string table_dir = "./test_simple";
    // mkdir(table_dir.c_str(), 0755);

    // MetaManager mm(table_dir);
    // mm.load();
    // FileManager fm(table_dir, mm.pageSize());
    // RecordManager rm(mm.columns());
    // QueryProcessor qp;

    const std::string data_root = "./data";
    mkdir(data_root.c_str(), 0755);
    DatabaseManager db("./data");

    std::cout << titleText << std::endl;
    std::cout << "Welcome to OurSQL. Enter SQL (type EXIT to quit):\n";
    std::string line;
    while (true) {
        std::cout << "OurSQL> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "EXIT" || line == "exit") break;
        db.exec(line);

        // Query q = qp.parse(line);
        // if (q.type == QueryType::INVALID) {
        //     std::cout << "Syntax error or unsupported command.\n";
        //     continue;
        // }
        // qp.exec(q, mm, fm, rm);
    }

    return 0;
}
