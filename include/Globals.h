#pragma once

#include <stdexcept>


constexpr unsigned int PAGE_SIZE = 4096;
constexpr unsigned int CHAR_MAXLEN = 127;

enum class DataType {
    INT,
    FLOAT,
    CHAR
};


inline const char* DataTypeToStr(DataType t) {
    switch (t) {
        case DataType::INT:   return "INT";
        case DataType::FLOAT: return "FLOAT";
        case DataType::CHAR:  return "CHAR";
        default: return "UNKNOWN";
    }
}

inline DataType StrToDataType(const std::string& str) {
    if (str == "INT") return DataType::INT;
    if (str == "FLOAT") return DataType::FLOAT;
    if (str == "CHAR") return DataType::CHAR;
    throw std::invalid_argument("Unknown DataType: " + str);
}
