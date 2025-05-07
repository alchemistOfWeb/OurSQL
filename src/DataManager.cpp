#include "FileDispatcher.h"
#include "DataManager.h"
#include "MetaManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <arpa/inet.h>

namespace OurSQL {

DataManager::DataManager(const std::string& dbPath)
    : m_metaMgr(dbPath), m_dataDir(dbPath + "/data/")
{}

bool DataManager::insertRow(const std::string& table, const Row& row) {
    
    if (!(m_metaMgr.getSchema(table) || m_metaMgr.loadSchema(table))) {
        std::cerr << "Error: no such table " + table << std::endl;
        return false;
    }

    const TableSchema* schema = m_metaMgr.getSchema(table);

    std::string path = m_dataDir + table + ".tbl";
    int fd = ::open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) return false;

    for (const auto& col : schema->columns) {
        auto it = row.find(col.name);
        if (it == row.end()) {
            ::close(fd);
            std::cerr << "Error: missing column" << std::endl;
            return false; // missing column
        }

        const std::string& val = it->second;
        if (col.type == ColumnType::INTEGER) {
            int32_t v = std::stoi(val);
            int32_t net_v = htonl(v);
            ::write(fd, &net_v, sizeof(int32_t));
        }
        else if (col.type == ColumnType::FLOAT) {
            float v = std::stof(val);
            ::write(fd, &v, sizeof(float)); // for simplicity: no endian conversion
        }
        else if (col.type == ColumnType::TEXT) {
            uint32_t len = val.size();
            uint32_t net_len = htonl(len);
            ::write(fd, &net_len, sizeof(uint32_t));
            ::write(fd, val.data(), len);
        }
    }

    ::close(fd);
    return true;
}

SelectResult DataManager::selectAllRows(const std::string& table) {
    SelectResult result;

    if (!(m_metaMgr.getSchema(table) || m_metaMgr.loadSchema(table))) {
        std::cerr << "Error: no such table " + table << std::endl;
        return result;
    }
    const TableSchema* schema = m_metaMgr.getSchema(table);

    result.columnNames.reserve(schema->columns.size());
    for (const auto& col : schema->columns) { // deep copying?
        result.columnNames.push_back(col.name);
    }
    std::cout << std::endl;

    std::string path = m_dataDir + table + ".tbl";

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error: cannot open file " + path << std::endl;
        return result;
    }

    bool eof = false;

    while (!eof) {
        Row row;
        for (const auto& col : schema->columns) {
            if (col.type == ColumnType::INTEGER) {
                int32_t v;
                ssize_t r = ::read(fd, &v, sizeof(v));
                if (r != sizeof(v)) {
                    eof = true;
                }
                row[col.name] = std::to_string(ntohl(v));
            }
            else if (col.type == ColumnType::FLOAT) {
                float v;
                ssize_t r = ::read(fd, &v, sizeof(v));
                if (r != sizeof(v)) {
                    eof = true;
                }
                row[col.name] = std::to_string(v);
            }
            else if (col.type == ColumnType::TEXT) {
                uint32_t len;
                ssize_t r1 = ::read(fd, &len, sizeof(len));
                if (r1 != sizeof(len)) {
                    eof = true;
                }
                len = ntohl(len);
                std::cout << "textlen " << len << std::endl;
                std::string val(len, '\0');
                ssize_t r2 = ::read(fd, val.data(), len);
                if (r2 != (ssize_t)len) {
                    eof = true;
                }
                std::cout << "text " << val << std::endl;
                row[col.name] = val;
            }
        }
        if (!eof) {
            result.rows.push_back(std::move(row));
        }
    }

    ::close(fd);
    return result;
}

} // namespace OurSQL
