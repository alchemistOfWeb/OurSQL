#pragma once
#include <string>
#include <vector>
#include <cstdint>

class FileManager {
public:
    FileManager(const std::string &table_path, uint32_t page_size);
    ~FileManager();

    // Read page page_no into buffer (with page_size)
    bool read_page(uint32_t page_no, std::vector<char> &buffer);
    // Write buffer into page_no
    bool write_page(uint32_t page_no, const std::vector<char> &buffer);
    
    uint32_t page_count() const; // (size of data.dat / page_size)
    // Sync file to disk
    void sync();

private:
    int fd_data_;
    std::string data_path_;
    uint32_t page_size_;
};