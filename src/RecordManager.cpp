#include "RecordManager.h"
#include "Globals.h"
#include <cstring>
#include <cassert>

// Page format:
// [2 bites used][2 bites total][2*128 offsets][128 flags][data]
// offset:  (from page start) for each record
// flag: 0 - alive, 1 - deleted
// data: [id][data...], for each record

constexpr size_t OFFS_START = 4; // after used/total
constexpr size_t FLAGS_START = OFFS_START + 2 * RecordManager::MAX_RECORDS_PER_PAGE;
constexpr size_t DATA_START  = FLAGS_START + RecordManager::MAX_RECORDS_PER_PAGE;

RecordManager::RecordManager(const std::vector<ColumnInfo>& columns)
    : columns_(columns) {}

size_t RecordManager::recordDataSize(const std::vector<std::string>& values) const {
    size_t sz = 2; // id
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].type == DataType::INT) {
            sz += sizeof(int32_t);
        } else if (columns_[i].type == DataType::FLOAT) {
            sz += sizeof(float);
        } else if (columns_[i].type == DataType::CHAR) {
            sz += 1 + 127; // 1 bite lenght, 127 - max (without \0)
        }
    }
    return sz;
}

size_t RecordManager::recordDataSize() const {
    // for empty value (in case we check limit)
    std::vector<std::string> dummy(columns_.size(), "");
    return recordDataSize(dummy);
}

std::vector<char> RecordManager::serialize(const std::vector<std::string>& values) const {
    std::vector<char> buf;
    buf.resize(recordDataSize(values), 0);
    uint16_t rid = 0; // no serializatino for id in serialize
    size_t p = 0;
    memcpy(buf.data(), &rid, 2);
    p += 2;
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].type == DataType::INT) {
            int32_t v = std::stoi(values[i]);
            memcpy(buf.data() + p, &v, sizeof(int32_t));
            p += sizeof(int32_t);
        } else if (columns_[i].type == DataType::FLOAT) {
            float v = std::stof(values[i]);
            memcpy(buf.data() + p, &v, sizeof(float));
            p += sizeof(float);
        } else if (columns_[i].type == DataType::CHAR) {
            uint8_t len = std::min<uint8_t>(values[i].size(), 127);
            memcpy(buf.data() + p, &len, 1); ++p;
            memcpy(buf.data() + p, values[i].c_str(), len);
            p += 127;
        }
    }
    return buf;
}

std::vector<std::string> RecordManager::deserialize(const char* data, size_t size) const {
    std::vector<std::string> result;
    size_t p = 2; // skip id
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].type == DataType::INT) {
            int32_t v = 0;
            memcpy(&v, data + p, sizeof(int32_t));
            p += sizeof(int32_t);
            result.push_back(std::to_string(v));
        } else if (columns_[i].type == DataType::FLOAT) {
            float v = 0;
            memcpy(&v, data + p, sizeof(float));
            p += sizeof(float);
            result.push_back(std::to_string(v));
        } else if (columns_[i].type == DataType::CHAR) {
            uint8_t len = data[p]; ++p;
            std::string s(data + p, data + p + len);
            result.push_back(s);
            p += 127;
        }
    }
    return result;
}

std::vector<RecordLocation> RecordManager::scanPage(const std::vector<char>& page) const {
    std::vector<RecordLocation> recs;
    uint16_t used = 0, total = 0;
    memcpy(&used, page.data(), 2);
    memcpy(&total, page.data() + 2, 2);

    for (uint16_t i = 0; i < total; ++i) {
        uint16_t off = 0;
        memcpy(&off, page.data() + OFFS_START + i * 2, 2);
        uint8_t flag = page[FLAGS_START + i];
        if (off == 0) continue;
        RecordLocation rl{ i, off, uint16_t(recordDataSize()), flag != 0 };
        recs.push_back(rl);
    }
    return recs;
}

bool RecordManager::insertRecord(std::vector<char>& page, const std::vector<std::string>& values, uint16_t& out_id) {
    // use header
    uint16_t used = 0, total = 0;
    memcpy(&used, page.data(), 2);
    memcpy(&total, page.data() + 2, 2);
    if (total >= MAX_RECORDS_PER_PAGE) return false;
    size_t rec_size = recordDataSize(values);

    // where we can place a record? — after the last or instead of deleted
    uint16_t slot = total;
    for (uint16_t i = 0; i < total; ++i) {
        uint8_t flag = page[FLAGS_START + i];
        if (flag != 0) { slot = i; break; }
    }
    size_t free_space = page.size() - DATA_START;
    size_t rec_pos = DATA_START + slot * rec_size;
    if (rec_pos + rec_size > page.size()) return false; // no space

    // id
    out_id = slot;
    std::vector<char> rec = serialize(values);
    memcpy(rec.data(), &out_id, 2);

    memcpy(page.data() + rec_pos, rec.data(), rec_size);

    uint16_t one = rec_pos;
    memcpy(page.data() + OFFS_START + slot * 2, &one, 2);
    page[FLAGS_START + slot] = 0;
    if (slot == total) {
        ++total;
    }
    ++used;
    memcpy(page.data(), &used, 2);
    memcpy(page.data() + 2, &total, 2);
    return true;
}

bool RecordManager::deleteRecord(std::vector<char>& page, uint16_t record_id) {
    uint16_t used = 0;
    memcpy(&used, page.data(), 2);
    uint8_t flag = page[FLAGS_START + record_id];
    if (flag != 0) return false; // already deleted
    page[FLAGS_START + record_id] = 1;
    --used;
    memcpy(page.data(), &used, 2);
    return true;
}

bool RecordManager::updateRecord(std::vector<char>& page, uint16_t record_id, const std::vector<std::string>& values) {
    uint8_t flag = page[FLAGS_START + record_id];
    if (flag != 0) return false; // deleted
    size_t rec_size = recordDataSize(values);
    size_t rec_pos = DATA_START + record_id * rec_size;
    std::vector<char> rec = serialize(values);
    memcpy(rec.data(), &record_id, 2);
    memcpy(page.data() + rec_pos, rec.data(), rec_size);
    return true;
}
