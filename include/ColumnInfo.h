#pragma once
#include <string>
#include "Globals.h"
#include <optional>

struct ColumnInfo {
    std::string name;
    DataType type;
    bool nullable = false;
    std::optional<std::string> default_value;
};