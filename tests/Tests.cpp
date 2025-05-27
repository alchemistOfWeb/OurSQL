#include "MetaManager.h"
#include "FileManager.h"
#include "RecordManager.h"
#include "QueryProcessor.h"
#include "Globals.h"
#include "ColumnInfo.h"
#include "DatabaseManager.h"
#include <cassert>
#include <fstream>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <iostream>


void testMetaManager() {
    const std::string dir = "./data/test_table_meta";
    mkdir(dir.c_str(), 0755);
    MetaManager mm(dir);
    mm.setTableName("meta_test");
    mm.setPageSize(PAGE_SIZE);
    mm.addColumn({"id", DataType::INT});
    mm.addColumn({"grade", DataType::FLOAT});
    mm.addColumn({"name", DataType::CHAR});
    mm.save();

    MetaManager mm2(dir);
    mm2.load();
    assert(mm2.tableName() == "meta_test");
    assert(mm2.pageSize() == PAGE_SIZE);
    assert(mm2.columns().size() == 3);
    assert(mm2.columns()[2].type == DataType::CHAR);
    // cleanup
    unlink((dir + "/metadata.meta").c_str());
    rmdir(dir.c_str());
    std::cout << "MetaManager tests passed" << std::endl;
}

void testFileManager() {
    const std::string dir = "./data/test_table";
    unlink((dir + "/data.dat").c_str());
    rmdir(dir.c_str());
    mkdir(dir.c_str(), 0755);
    uint32_t ps = 4096;
    FileManager fm(dir, ps);
    // in the beginning must be 0 pages
    assert(fm.page_count() == 0);
    // write 0 page
    std::vector<char> buf(ps, 'A');
    assert(fm.write_page(0, buf));
    assert(fm.page_count() == 1);
    // read
    std::vector<char> read_buf;
    assert(fm.read_page(0, read_buf));
    assert(read_buf == buf);
    // write
    std::vector<char> buf2(ps, 'B');
    assert(fm.write_page(0, buf2));
    assert(fm.read_page(0, read_buf));
    assert(read_buf == buf2);
    // cleanup
    unlink((dir + "/data.dat").c_str());
    rmdir(dir.c_str());
    std::cout << "FileManager tests passed" << std::endl;
}

void testRecordManager() {
    // columns: id INT, grade FLOAT, name CHAR
    
    std::vector<ColumnInfo> cols = {
        {"id", DataType::INT},
        {"grade", DataType::FLOAT},
        {"name", DataType::CHAR}
    };

    RecordManager rm(cols);

    std::vector<char> page(4096, 0);
    uint16_t rid;
    assert(rm.insertRecord(page, {"1", "4.5", "Petrov"}, rid));
    assert(rid == 0);

    assert(rm.insertRecord(page, {"2", "3.8", "Sidorov"}, rid));
    assert(rid == 1);

    auto recs = rm.scanPage(page);
    assert(recs.size() == 2);

    // read first record
    auto vals = rm.deserialize(&page[recs[0].offset], recs[0].size);
    assert(vals[0] == "1");
    assert(vals[1].substr(0,3) == "4.5");
    assert(vals[2] == "Petrov");

    // Delete first record
    assert(rm.deleteRecord(page, 0));
    auto recs2 = rm.scanPage(page);
    assert(recs2[0].deleted == true);
    assert(recs2[1].deleted == false);

    // update second record
    assert(rm.updateRecord(page, 1, {"2", "5.0", "Ivanov"}));
    auto vals2 = rm.deserialize(&page[recs2[1].offset], recs2[1].size);
    assert(vals2[2] == "Ivanov");

    std::cout << "RecordManager tests passed" << std::endl;
}

void testQueryProcessor() {
    QueryProcessor qp;
    auto q1 = qp.parse("SELECT id, name FROM students WHERE id = 42");
    assert(q1.type == QueryType::SELECT);
    assert(q1.table == "students");
    assert(q1.columns.size() == 2 && q1.columns[0] == "id" && q1.columns[1] == "name");
    assert(q1.where_col == "id" && q1.where_val == "42");

    auto q2 = qp.parse("INSERT INTO books (id, title) VALUES (1, 'test')");
    assert(q2.type == QueryType::INSERT);
    assert(q2.table == "books");
    assert(q2.columns.size() == 2);
    assert(q2.values.size() == 2);

    auto q3 = qp.parse("UPDATE books SET title = 'test2' WHERE id = 1");
    assert(q3.type == QueryType::UPDATE);
    assert(q3.table == "books");
    assert(q3.columns[0] == "title");
    assert(q3.values[0] == "'test2'");
    assert(q3.where_col == "id");

    auto q4 = qp.parse("DELETE FROM books WHERE id = 1");
    assert(q4.type == QueryType::DELETE);
    assert(q4.table == "books");
    assert(q4.where_col == "id");

    auto q5 = qp.parse("CREATE TABLE users (id INT, name CHAR)");
    assert(q5.type == QueryType::CREATE);
    assert(q5.table == "users");

    std::cout << "QueryProcessor tests passed" << std::endl;
}

void testQueryProcessorExec() {
    const std::string dir = "./data/test_simple";
    mkdir(dir.c_str(), 0755);
    MetaManager mm(dir);
    mm.setTableName("test_simple");
    mm.setPageSize(4096);
    mm.addColumn({"id", DataType::INT});
    mm.addColumn({"score", DataType::FLOAT});
    mm.addColumn({"name", DataType::CHAR});
    mm.save();

    FileManager fm(dir, 4096);
    RecordManager rm(mm.columns());
    QueryProcessor qp;

    // INSERT
    auto q1 = qp.parse("INSERT INTO test_simple (id,score,name) VALUES (1, 4.9, Ivan)");
    assert(qp.exec(q1, mm, fm, rm));
    auto q2 = qp.parse("INSERT INTO test_simple (id,score,name) VALUES (2, 2.5, Petr)");
    assert(qp.exec(q2, mm, fm, rm));

    // SELECT *
    auto qsel = qp.parse("SELECT * FROM test_simple");
    assert(qp.exec(qsel, mm, fm, rm)); // must output 2 records

    // UPDATE
    auto qup = qp.parse("UPDATE test_simple SET score = 3.7 WHERE name = Ivan");
    assert(qp.exec(qup, mm, fm, rm));
    // SELECT c where
    auto qsel2 = qp.parse("SELECT id,score FROM test_simple WHERE name = Ivan");
    assert(qp.exec(qsel2, mm, fm, rm)); // score must be 3.7

    // DELETE
    auto qdel = qp.parse("DELETE FROM test_simple WHERE name = Ivan");
    assert(qp.exec(qdel, mm, fm, rm));
    auto qsel3 = qp.parse("SELECT * FROM test_simple");
    assert(qp.exec(qsel3, mm, fm, rm)); // only Petr

    // cleanup
    unlink((dir + "/data.dat").c_str());
    unlink((dir + "/metadata.meta").c_str());
    rmdir(dir.c_str());
    std::cout << "QueryProcessor::exec tests passed" << std::endl;
}

void testDatabaseManager() {

    const std::string data_root = "./data";
    mkdir(data_root.c_str(), 0755);

    DatabaseManager db(data_root);

    // CREATE TABLE
    assert(db.exec("CREATE TABLE users (id INT, name CHAR, score FLOAT)"));

    // INSERT
    assert(db.exec("INSERT INTO users (id,name,score) VALUES (1, 'Petya', 4.5)"));
    assert(db.exec("INSERT INTO users (id,name,score) VALUES (2, 'Masha', 3.7)"));
    assert(db.exec("INSERT INTO users (id,name,score) VALUES (3, 'Kolya', 5.0)"));

    // SELECT * FROM users
    std::cout << "--- USERS TABLE ---\n";
    assert(db.exec("SELECT * FROM users"));

    // UPDATE (change score of Petya)
    assert(db.exec("UPDATE users SET score = 4.9 WHERE name = 'Petya'"));
    std::cout << "--- PETYA UPDATED ---\n";
    assert(db.exec("SELECT id,score FROM users WHERE name = 'Petya'"));

    // DELETE (delete Masha)
    assert(db.exec("DELETE FROM users WHERE name = 'Masha'"));
    std::cout << "--- AFTER DELETE MASHA ---\n";
    assert(db.exec("SELECT * FROM users"));

    // second table books and check independance
    assert(db.exec("CREATE TABLE books (id INT, title CHAR)"));
    assert(db.exec("INSERT INTO books (id,title) VALUES (1, 'War and Peace')"));
    assert(db.exec("INSERT INTO books (id,title) VALUES (2, '1984')"));
    std::cout << "--- BOOKS TABLE ---\n";
    assert(db.exec("SELECT * FROM books"));

    // Cleanup: 
    unlink("./data/users/data.dat");
    unlink("./data/users/metadata.meta");
    rmdir("./data/users");
    unlink("./data/books/data.dat");
    unlink("./data/books/metadata.meta");
    rmdir("./data/books");
    rmdir("./data");

    std::cout << "DatabaseManager tests passed" << std::endl;
}

void testNullAndDefault() {
    DatabaseManager db("./data");
    assert(db.exec("CREATE TABLE test (id INT, name CHAR DEFAULT 'anon', score FLOAT)"));
    assert(db.exec("INSERT INTO test (id,score) VALUES (1, 2.2)"));
    assert(db.exec("INSERT INTO test (id,name,score) VALUES (2,NULL,5.0)"));
    assert(db.exec("SELECT * FROM test"));
    // Expecting: name will be 'anon' and 'NULL'
    // cleanup:
    unlink("./data/test/data.dat");
    unlink("./data/test/metadata.meta");
    rmdir("./data/test");
    rmdir("./data");
    std::cout << "NULL/DEFAULT tests passed\n";
}

int main() {
    mkdir("data/", 0755);
    testFileManager();
    testMetaManager();
    testRecordManager();
    testQueryProcessor();
    testQueryProcessorExec();
    testDatabaseManager();
    std::cout << "All tests passed" << std::endl;
    return 0;
}