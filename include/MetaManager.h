#pragma once
#include "ColumnInfo.h"
#include <string>
#include <vector>

class MetaManager {
public:
    explicit MetaManager(const std::string &table_path);

    void load();
    void save() const;

    const std::string& tableName() const;
    unsigned int pageSize() const;
    const std::vector<ColumnInfo>& columns() const;

    void setTableName(const std::string &name);
    void setPageSize(unsigned int size);
    void addColumn(const ColumnInfo &col);

private:
    std::string dirPath_;
    std::string filePath_;

    std::string table_name_;
    unsigned int page_size_;
    std::vector<ColumnInfo> columns_;
};
