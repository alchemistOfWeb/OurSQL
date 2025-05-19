#include "MetaManager.h"
#include "Globals.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <sstream>
#include <cstring>

MetaManager::MetaManager(const std::string &table_path)
    : dirPath_(table_path),
      filePath_(table_path + "/metadata.meta"),
      page_size_(PAGE_SIZE) {}

void MetaManager::save() const {
    int fd = open(filePath_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) throw std::runtime_error("Cannot open metadata.meta for writing");

    std::ostringstream oss;
    oss << "table_name=" << table_name_ << std::endl;
    oss << "page_size=" << page_size_ << std::endl;
    for (const auto &c : columns_) {
        oss << "column=" << c.name << ":" << DataTypeToStr(c.type) << std::endl;
    }

    std::string data = oss.str();
    ssize_t written = write(fd, data.data(), data.size());
    if (written != static_cast<ssize_t>(data.size())) {
        close(fd);
        throw std::runtime_error("Failed to write metadata.meta");
    }
    fsync(fd);
    close(fd);
}

void MetaManager::load() {
    int fd = open(filePath_.c_str(), O_RDONLY);
    if (fd < 0) throw std::runtime_error("Cannot open metadata.meta for reading");

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        throw std::runtime_error("fstat failed");
    }

    std::vector<char> buf(st.st_size);
    ssize_t r = read(fd, buf.data(), st.st_size);
    if (r != static_cast<ssize_t>(st.st_size)) {
        close(fd);
        throw std::runtime_error("Failed to read metadata.meta");
    }
    close(fd);

    std::istringstream iss(std::string(buf.data(), buf.size()));
    std::string line;
    columns_.clear();

    const char *key_table = "table_name=";
    const char *key_size = "page_size=";
    const char *key_col = "column=";

    while (std::getline(iss, line)) {
        if (line.rfind(key_table, 0) == 0) {
            table_name_ = line.substr(strlen(key_table));
        } else if (line.rfind(key_size, 0) == 0) {
            page_size_ = static_cast<unsigned int>(std::stoul(line.substr(strlen(key_size))));
        } else if (line.rfind(key_col, 0) == 0) {
            auto kv = line.substr(strlen(key_col));
            auto pos = kv.find(':');
            if (pos != std::string::npos) {
                columns_.push_back({kv.substr(0, pos), StrToDataType(kv.substr(pos + 1))});
            }
        }
    }
}

const std::string& MetaManager::tableName() const { return table_name_; }
unsigned int MetaManager::pageSize() const { return page_size_; }
const std::vector<ColumnInfo>& MetaManager::columns() const { return columns_; }

void MetaManager::setTableName(const std::string &name) { table_name_ = name; }
void MetaManager::setPageSize(unsigned int size) { page_size_ = size; }
void MetaManager::addColumn(const ColumnInfo &col) { columns_.push_back(col); }
