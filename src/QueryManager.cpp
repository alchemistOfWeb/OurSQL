#include "QueryManager.h"
#include "DataManager.h"
#include "MetaManager.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace OurSQL {

QueryManager::QueryManager(const std::string& dbPath)
    : m_dataMgr(dbPath)
{}

void QueryManager::handleQuery(const std::string& query) {
    std::istringstream instr(query);
    std::string cmd;
    instr >> cmd;
    
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "insert") {
        handleInsert(query);
    } else if (cmd == "select") {
        handleSelect(query);
    } else {
        std::cout << "Unknown query.\n";
    }
}

void QueryManager::handleInsert(const std::string& query) { 
    std::regex pattern(R"(insert\s+into\s+(\w+)\s*\(([^)]+)\)\s*values\s*\(([^)]+)\);?)", std::regex::icase);
    std::smatch match;
    if (!std::regex_match(query, match, pattern)) {
        std::cerr << "Error: invalid INSERT syntax\n";
        return;
    }

    std::string table = match[1];
    std::string colsStr = match[2];
    std::string valsStr = match[3];

    std::vector<std::string> cols;
    std::stringstream colStream(colsStr);
    std::string col;
    while (std::getline(colStream, col, ',')) {
        cols.push_back(std::string(
            std::find_if_not(col.begin(), col.end(), isspace),
            std::find_if(col.rbegin(), col.rend(), [](char c) { return !isspace(c); }).base()
        ));
    }

    std::vector<std::string> vals;
    std::stringstream valStream(valsStr);
    std::string val;
    while (std::getline(valStream, val, ',')) {
        val.erase(std::remove(val.begin(), val.end(), '\''), val.end()); // remove quotes
        val.erase(std::remove_if(val.begin(), val.end(), isspace), val.end());
        vals.push_back(val);
    }

    if (cols.size() != vals.size()) {
        std::cerr << "Column count doesn't match value count\n";
        return;
    }

    Row row;
    for (size_t i = 0; i < cols.size(); ++i) {
        row[cols[i]] = vals[i];
    }

    if (!m_dataMgr.insertRow(table, row)) {
        std::cerr << "Failed to insert row\n";
    } else {
        std::cout << "Row inserted into '" << table << "'\n";
    }
}

void QueryManager::handleSelect(const std::string& query) {
    std::regex pattern(R"(select\s+\*\s+from\s+(\w+);?)", std::regex::icase);
    std::smatch match;
    if (!std::regex_match(query, match, pattern)) {
        std::cerr << "Invalid SELECT syntax\n";
        return;
    }

    std::string table = match[1];
    SelectResult result = m_dataMgr.selectAllRows(table);

    if (result.rows.empty()) {
        std::cout << "No data in table '" << table << "'\n";
        return;
    }

    for (const auto& colName : result.columnNames) {
        std::cout << colName << "\t";
    }
    std::cout << "\n";

    for (const auto& row : result.rows) {
        for (const auto& colName : result.columnNames) {
            std::cout << row.at(colName) << "\t";
        }
        std::cout << "\n";
    }
}

} // namespace OurSQL
