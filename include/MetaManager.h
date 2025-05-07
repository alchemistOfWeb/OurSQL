#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace OurSQL {

enum class ColumnType { INTEGER, FLOAT, TEXT };

struct ColumnDef {
    std::string name;
    ColumnType type;
};

struct TableSchema {
    std::string tableName;
    std::vector<ColumnDef> columns;
    std::string primaryKey;
    std::vector<std::string> indices;
};

void trim(std::string& s);

class MetaManager {
public:
    explicit MetaManager(const std::string& dbPath);
    // Load scheme of a table data/<table>.meta
    bool loadSchema(const std::string& table);

    const TableSchema* getSchema(const std::string& table) const;

private:
    std::string m_metaDir;  // dbPath + "/data/"
    std::unordered_map<std::string, TableSchema> m_schemas;
    ColumnType parseType(const std::string& s) const;
};

} // namespace OurSQL
