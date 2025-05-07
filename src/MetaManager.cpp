#include "MetaManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <cctype>
#include <sstream>
#include <iostream>


namespace OurSQL {

void trim(std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    size_t e = s.find_last_not_of(" \t\r\n");
    if (b == std::string::npos) { s.clear(); }
    else { s = s.substr(b, e - b + 1); }
}

ColumnType MetaManager::parseType(const std::string& s) const {
    if (s == "INTEGER") return ColumnType::INTEGER;
    if (s == "FLOAT")   return ColumnType::FLOAT;
    if (s == "TEXT")    return ColumnType::TEXT;
    return ColumnType::TEXT;
}

MetaManager::MetaManager(const std::string& dbPath)
    : m_metaDir(dbPath + "/data/") {}


bool MetaManager::loadSchema(const std::string& table) {
    const std::string path = m_metaDir + table + ".meta";

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;

    struct stat st; // for getting file size
    if (fstat(fd, &st) != 0) {
        ::close(fd);
        std::cerr << "Error: Failed to read file stats." << std::endl;
        return false;
    }

    std::string content;
    content.resize(st.st_size);

    ssize_t rd = ::read(fd, content.data(), st.st_size);
    ::close(fd);

    if (rd != st.st_size) return false; // read not enough data?

    TableSchema schema;
    schema.tableName = table;

    std::istringstream instr(content);
    std::string line;
    while (std::getline(instr, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream parts(line);
        std::string key;
        parts >> key;

        if (key == "TABLE") {
            // already defined above
        } else if (key == "COLUMNS") {
            std::string colDef;
            while (parts >> colDef) {
                auto pos = colDef.find(':');
                if (pos == std::string::npos) continue;
                ColumnDef cd;
                cd.name = colDef.substr(0, pos);
                std::string t = colDef.substr(pos + 1);
                cd.type = parseType(t);
                schema.columns.push_back(std::move(cd));
            }
        } else if (key == "PRIMARY_KEY") {
            parts >> schema.primaryKey;
        } else if (key == "INDEX") {
            std::string idx;
            while (parts >> idx) {
                schema.indices.push_back(idx);
            }
        }
        // ignore other keys
    }

    m_schemas.emplace(table, std::move(schema));
    return true;
};

const TableSchema* MetaManager::getSchema(const std::string& table) const {
    auto it = m_schemas.find(table);
    if (it == m_schemas.end()) return nullptr;
    return &it->second;
};


} // namespace OurSQL
