#pragma once
#include "MetaManager.h"
#include "FileManager.h"
#include "RecordManager.h"
#include "QueryProcessor.h"
#include <map>
#include <memory>
#include <sys/stat.h>

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& data_root) : root_(data_root) {}
    bool exec(const std::string& sql);

private:
    std::string root_;

    
    std::map<std::string, std::shared_ptr<MetaManager>> meta_;
    std::map<std::string, std::shared_ptr<FileManager>> file_;
    std::map<std::string, std::shared_ptr<RecordManager>> record_;

    QueryProcessor qp_;

    // helpers
    std::string table_dir(const std::string& table) const {
        return root_ + "/" + table;
    }
    
    std::shared_ptr<MetaManager> getMeta(const std::string& table) {
        if (!meta_.count(table)) {
            auto ptr = std::make_shared<MetaManager>(table_dir(table));
            ptr->load();
            meta_[table] = ptr;
        }
        return meta_[table];
    }
    std::shared_ptr<FileManager> getFile(const std::string& table) {
        if (!file_.count(table)) {
            auto mm = getMeta(table);
            auto ptr = std::make_shared<FileManager>(table_dir(table), mm->pageSize());
            file_[table] = ptr;
        }
        return file_[table];
    }
    std::shared_ptr<RecordManager> getRecord(const std::string& table) {
        if (!record_.count(table)) {
            auto mm = getMeta(table);
            auto ptr = std::make_shared<RecordManager>(mm->columns());
            record_[table] = ptr;
        }
        return record_[table];
    }
    void createTable(const Query& q);
};
