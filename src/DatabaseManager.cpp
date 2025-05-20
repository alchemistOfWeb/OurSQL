#include "DatabaseManager.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <sstream>

void DatabaseManager::createTable(const Query& q) {
    // for example: CREATE TABLE users (id INT, name CHAR)
    std::string dir = table_dir(q.table);
    mkdir(dir.c_str(), 0755);
    MetaManager mm(dir);
    mm.setTableName(q.table);
    mm.setPageSize(4096); 
    // parse cols (name type, ...)
    auto cols_def = q.raw_sql.substr(q.raw_sql.find('(') + 1);
    cols_def = cols_def.substr(0, cols_def.find(')'));
    std::vector<std::string> tokens;
    std::istringstream iss(cols_def);
    std::string token;
    while (std::getline(iss, token, ',')) {
        // divide name and type
        std::istringstream tkn(token);
        std::string cname, ctype;
        tkn >> cname >> ctype;
        std::transform(ctype.begin(), ctype.end(), ctype.begin(), ::toupper);
        DataType t;
        if (ctype == "INT") t = DataType::INT;
        else if (ctype == "FLOAT") t = DataType::FLOAT;
        else if (ctype == "CHAR") t = DataType::CHAR;
        else throw std::runtime_error("Unknown type: " + ctype);
        mm.addColumn({cname, t});
    }
    mm.save();
    std::cout << "Table " << q.table << " created.\n";
    // init first empty page:
    FileManager fm(dir, mm.pageSize());
    std::vector<char> empty(mm.pageSize(), 0);
    fm.write_page(0, empty);
}

bool DatabaseManager::exec(const std::string& sql) {
    Query q = qp_.parse(sql);
    if (q.type == QueryType::INVALID) {
        std::cout << "Syntax error or unsupported command.\n";
        return false;
    }
    if (q.type == QueryType::CREATE) {
        createTable(q);
        return true;
    }
    
    if (q.table.empty()) {
        std::cerr << "No table specified in query!\n";
        return false;
    }
    
    auto mm = getMeta(q.table);
    auto fm = getFile(q.table);
    auto rm = getRecord(q.table);
    return qp_.exec(q, *mm, *fm, *rm);
}
