#pragma once

#include <cstddef>
#include <string>

namespace OurSQL {

class FileDispatcher {
public:
    static bool append(const std::string& path, const void* buf, std::size_t size);

    static bool readAll(const std::string& path,
                        void (*callback)(const void*, std::size_t));
};

} // namespace OurSQL