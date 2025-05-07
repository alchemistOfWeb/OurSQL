#pragma once

#include <string>
#include <vector>
#include "MetaManager.h"
#include <unordered_map>

namespace OurSQL {

typedef std::unordered_map<std::string, std::string> Row;

struct SelectResult {
    std::vector<std::string> columnNames;
    std::vector<Row> rows;
};

class DataManager {
public:
    explicit DataManager(const std::string& dbPath);

    bool insertRow(const std::string& table, const Row& row);

    SelectResult selectAllRows(const std::string& table);

private:
    MetaManager m_metaMgr;
    std::string m_dataDir;
};

} // namespace OurSQL
