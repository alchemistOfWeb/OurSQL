#include "FileManager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>

#define DEFAULT_PAGE_SIZE 4096

FileManager::FileManager(const std::string &table_path, uint32_t page_size=DEFAULT_PAGE_SIZE)
    : page_size_(page_size) {
    data_path_ = table_path + "/data.dat";
    fd_data_ = ::open(data_path_.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd_data_ < 0) throw std::runtime_error("Cannot open data.dat");
}

FileManager::~FileManager() {
    sync();
    ::close(fd_data_);
}

bool FileManager::read_page(uint32_t page_no, std::vector<char> &buffer) {
    buffer.resize(page_size_);
    off_t offset = static_cast<off_t>(page_no) * page_size_;
    ssize_t r = pread(fd_data_, buffer.data(), page_size_, offset);
    return r == static_cast<ssize_t>(page_size_);
}

bool FileManager::write_page(uint32_t page_no, const std::vector<char> &buffer) {
    if (buffer.size() != page_size_) return false;
    off_t offset = static_cast<off_t>(page_no) * page_size_;
    ssize_t w = pwrite(fd_data_, buffer.data(), page_size_, offset);
    return w == static_cast<ssize_t>(page_size_);
}

uint32_t FileManager::page_count() const {
    struct stat st;
    if (fstat(fd_data_, &st) < 0) throw std::runtime_error("fstat failed");
    return static_cast<uint32_t>(st.st_size / page_size_);
}

void FileManager::sync() {
    ::fsync(fd_data_);
}