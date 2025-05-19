#pragma once
#include <string>
#include <vector>
#include "MetaManager.h"
#include "FileManager.h"
#include "RecordManager.h"

enum class QueryType { SELECT, INSERT, UPDATE, DELETE, CREATE, INVALID };

struct Query {
    QueryType type;
    std::string table;
    std::vector<std::string> columns;    // for SELECT, INSERT
    std::vector<std::string> values;     // for INSERT, UPDATE
    std::string where_col;
    std::string where_val;
    std::string raw_sql;
};

class QueryProcessor {
public:
    Query parse(const std::string& sql) const;
    bool exec(const Query& q, MetaManager& mm, FileManager& fm, RecordManager& rm);
};
