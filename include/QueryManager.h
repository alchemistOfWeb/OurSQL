#pragma once

#include <string>
#include "DataManager.h"

namespace OurSQL {

class QueryManager {
public:
    explicit QueryManager(const std::string& dbPath);

    // Handle one text command
    void handleQuery(const std::string& query);

private:
    void handleInsert(const std::string& query);
    void handleSelect(const std::string& query);

    DataManager m_dataMgr;
    // MetaManager* m_metaMgr;
};

} // namespace OurSQL
