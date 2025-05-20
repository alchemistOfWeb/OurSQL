#pragma once
#include "ColumnInfo.h"
#include <vector>
#include <string>
#include <cstdint>

struct RecordLocation {
    uint16_t id;
    uint16_t offset;
    uint16_t size;
    bool deleted;
};

class RecordManager {
public:
    RecordManager(const std::vector<ColumnInfo>& columns);

    
    std::vector<char> serialize(const std::vector<std::string>& values) const;
    std::vector<std::string> deserialize(const char* data, size_t size) const;

    
    std::vector<RecordLocation> scanPage(const std::vector<char>& page) const;

    // CRUD 
    bool insertRecord(std::vector<char>& page, const std::vector<std::string>& values, uint16_t& out_id);
    bool deleteRecord(std::vector<char>& page, uint16_t record_id);
    bool updateRecord(std::vector<char>& page, uint16_t record_id, const std::vector<std::string>& values);

    
    static constexpr size_t PAGE_HEADER_SIZE = 8; // 2 bites used_records + 2 bites total_records + 2*max_records offsets + max_records flags
    static constexpr size_t MAX_RECORDS_PER_PAGE = 128;

private:
    std::vector<ColumnInfo> columns_;
    size_t recordDataSize(const std::vector<std::string>& values) const;
    size_t recordDataSize() const;
};
