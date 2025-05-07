#include "FileDispatcher.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace OurSQL {

bool FileDispatcher::append(
    const std::string& path, const void* buf, std::size_t size) {
        
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return false;
    ssize_t written = ::write(fd, buf, size);
    ::close(fd);
    return written == static_cast<ssize_t>(size);
}

bool FileDispatcher::readAll(
    const std::string& path,
    void (*callback)(const void*, std::size_t)) {
        
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;
    constexpr std::size_t BUF_SIZE = 4096;
    char buffer[BUF_SIZE];
    ssize_t bytes;

    while ((bytes = ::read(fd, buffer, BUF_SIZE)) > 0) {
        callback(buffer, static_cast<std::size_t>(bytes));
    }
    
    ::close(fd);
    return bytes == 0;
}

} // namespace OurSQL
