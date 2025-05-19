#include "QueryProcessor.h"
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

static void trim(std::string& s) {
    while (!s.empty() && std::isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace(s.back()))  s.pop_back();
}

Query QueryProcessor::parse(const std::string& sql) const {
    Query q;
    q.raw_sql = sql;
    std::istringstream iss(sql);
    std::string word;
    iss >> word;
    std::transform(word.begin(), word.end(), word.begin(), ::toupper);
    if (word == "SELECT") {
        q.type = QueryType::SELECT;
        std::string colstr;
        getline(iss, colstr, 'F'); // before FROM
        trim(colstr);
        if (colstr.front() == '*') {
            q.columns.clear();
        } else {
            std::replace(colstr.begin(), colstr.end(), ',', ' ');
            std::istringstream cstream(colstr);
            std::string col;
            while (cstream >> col) q.columns.push_back(col);
        }
        std::string fromWord, table;
        iss >> fromWord >> table;
        q.table = table;
        std::string whereWord, whereCol, eq, whereVal;
        if (iss >> whereWord && whereWord == "WHERE" && iss >> whereCol >> eq >> whereVal) {
            q.where_col = whereCol;
            q.where_val = whereVal;
        }
    } else if (word == "INSERT") {
        q.type = QueryType::INSERT;
        std::string intoWord, table, vals;
        iss >> intoWord >> table >> vals;
        q.table = table;
        size_t start = sql.find('(');
        size_t end   = sql.find(')');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string cols = sql.substr(start + 1, end - start - 1);
            std::replace(cols.begin(), cols.end(), ',', ' ');
            std::istringstream cstream(cols);
            std::string col;
            while (cstream >> col) q.columns.push_back(col);
        }
        size_t vpos = sql.find("VALUES");
        if (vpos != std::string::npos) {
            size_t vstart = sql.find('(', vpos);
            size_t vend   = sql.find(')', vstart);
            if (vstart != std::string::npos && vend != std::string::npos) {
                std::string vals = sql.substr(vstart + 1, vend - vstart - 1);
                std::replace(vals.begin(), vals.end(), ',', ' ');
                std::istringstream vstream(vals);
                std::string val;
                while (vstream >> val) q.values.push_back(val);
            }
        }
    } else if (word == "UPDATE") {
        q.type = QueryType::UPDATE;
        std::string table;
        iss >> table;
        q.table = table;
        std::string setWord, col, eq, val;
        iss >> setWord >> col >> eq >> val;
        if (setWord == "SET") {
            q.columns.push_back(col);
            q.values.push_back(val);
        }
        std::string whereWord, whereCol, whereEq, whereVal;
        if (iss >> whereWord && whereWord == "WHERE" && iss >> whereCol >> whereEq >> whereVal) {
            q.where_col = whereCol;
            q.where_val = whereVal;
        }
    } else if (word == "DELETE") {
        q.type = QueryType::DELETE;
        std::string fromWord, table;
        iss >> fromWord >> table;
        q.table = table;
        std::string whereWord, whereCol, eq, whereVal;
        if (iss >> whereWord && whereWord == "WHERE" && iss >> whereCol >> eq >> whereVal) {
            q.where_col = whereCol;
            q.where_val = whereVal;
        }
    } else if (word == "CREATE") {
        q.type = QueryType::CREATE;
        std::string tableWord, table, rest;
        iss >> tableWord >> table;
        q.table = table;
        // columns pass (DDL-parser not in this version)
    } else {
        q.type = QueryType::INVALID;
    }
    return q;
}


bool QueryProcessor::exec(const Query& q, MetaManager& mm, FileManager& fm, RecordManager& rm) {
    if (q.type == QueryType::INSERT) {
        // NOW but not always: insert in the last page.
        uint32_t page_no = fm.page_count() ? fm.page_count() - 1 : 0;
        std::vector<char> page(mm.pageSize(), 0);
        if (fm.page_count() == 0 || !fm.read_page(page_no, page)) {
            // new page.
            page.assign(mm.pageSize(), 0);
        }
        uint16_t out_id = 0;
        if (!rm.insertRecord(page, q.values, out_id)) {
            // no space, create new page.
            page.assign(mm.pageSize(), 0);
            page_no = fm.page_count();
            if (!rm.insertRecord(page, q.values, out_id)) {
                std::cerr << "Insert failed: record too large\n";
                return false;
            }
        }
        fm.write_page(page_no, page);
        std::cout << "Inserted into " << mm.tableName() << " record_id=" << out_id << " page=" << page_no << "\n";
        return true;
    }
    if (q.type == QueryType::SELECT) {
        // Full Scan!
        for (uint32_t p = 0; p < fm.page_count(); ++p) {
            std::vector<char> page;
            if (!fm.read_page(p, page)) continue;
            auto recs = rm.scanPage(page);
            for (const auto& rec : recs) {
                if (rec.deleted) continue;
                auto vals = rm.deserialize(&page[rec.offset], rec.size);
                // filter by where (only = )
                bool match = true;
                if (!q.where_col.empty()) {
                    for (size_t ci = 0; ci < mm.columns().size(); ++ci) {
                        if (mm.columns()[ci].name == q.where_col) {
                            if (vals[ci] != q.where_val) match = false;
                        }
                    }
                }
                if (match) {
                    // Show only necessary columns
                    if (q.columns.empty()) {
                        // all
                        for (auto& v : vals) std::cout << v << " ";
                    } else {
                        for (auto& col : q.columns) {
                            for (size_t ci = 0; ci < mm.columns().size(); ++ci)
                                if (mm.columns()[ci].name == col)
                                    std::cout << vals[ci] << " ";
                        }
                    }
                    std::cout << "\n";
                }
            }
        }
        return true;
    }
    if (q.type == QueryType::DELETE) {
        uint32_t deleted = 0;
        for (uint32_t p = 0; p < fm.page_count(); ++p) {
            std::vector<char> page;
            if (!fm.read_page(p, page)) continue;
            auto recs = rm.scanPage(page);
            bool page_changed = false;
            for (const auto& rec : recs) {
                if (rec.deleted) continue;
                auto vals = rm.deserialize(&page[rec.offset], rec.size);
                bool match = false;
                if (!q.where_col.empty()) {
                    for (size_t ci = 0; ci < mm.columns().size(); ++ci) {
                        if (mm.columns()[ci].name == q.where_col && vals[ci] == q.where_val) {
                            match = true;
                        }
                    }
                }
                if (match) {
                    rm.deleteRecord(page, rec.id);
                    ++deleted;
                    page_changed = true;
                }
            }
            if (page_changed) fm.write_page(p, page);
        }
        std::cout << "Deleted " << deleted << " rows\n";
        return true;
    }
    if (q.type == QueryType::UPDATE) {
        uint32_t updated = 0;
        for (uint32_t p = 0; p < fm.page_count(); ++p) {
            std::vector<char> page;
            if (!fm.read_page(p, page)) continue;
            auto recs = rm.scanPage(page);
            bool page_changed = false;
            for (const auto& rec : recs) {
                if (rec.deleted) continue;
                auto vals = rm.deserialize(&page[rec.offset], rec.size);
                bool match = false;
                if (!q.where_col.empty()) {
                    for (size_t ci = 0; ci < mm.columns().size(); ++ci) {
                        if (mm.columns()[ci].name == q.where_col && vals[ci] == q.where_val) {
                            match = true;
                        }
                    }
                }
                if (match) {
                    // values: update only concrete cols
                    std::vector<std::string> newvals = vals;
                    for (size_t ci = 0; ci < mm.columns().size(); ++ci)
                        if (mm.columns()[ci].name == q.columns[0])
                            newvals[ci] = q.values[0];
                    rm.updateRecord(page, rec.id, newvals);
                    ++updated;
                    page_changed = true;
                }
            }
            if (page_changed) fm.write_page(p, page);
        }
        std::cout << "Updated " << updated << " rows\n";
        return true;
    }
    std::cerr << "Unsupported or invalid query type\n";
    return false;
}
