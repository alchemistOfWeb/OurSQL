CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -I./include
LDFLAGS =

TARGET = run_tests
MAIN_TARGET = main

all: $(TARGET) $(MAIN_TARGET)

FileManager.o: src/FileManager.cpp include/FileManager.h
	$(CXX) $(CXXFLAGS) -c src/FileManager.cpp -o FileManager.o

RecordManager.o: src/RecordManager.cpp include/RecordManager.h
	$(CXX) $(CXXFLAGS) -c src/RecordManager.cpp -o RecordManager.o

MetaManager.o: src/MetaManager.cpp include/MetaManager.h
	$(CXX) $(CXXFLAGS) -c src/MetaManager.cpp -o MetaManager.o

QueryProcessor.o: src/QueryProcessor.cpp include/QueryProcessor.h
	$(CXX) $(CXXFLAGS) -c src/QueryProcessor.cpp -o QueryProcessor.o
	
DatabaseManager.o: src/DatabaseManager.cpp include/DatabaseManager.h
	$(CXX) $(CXXFLAGS) -c src/DatabaseManager.cpp -o DatabaseManager.o

Tests.o: tests/Tests.cpp include/FileManager.h include/MetaManager.h include/QueryProcessor.h include/DatabaseManager.h include/RecordManager.h include/ColumnInfo.h include/Globals.h
	$(CXX) $(CXXFLAGS) -c tests/Tests.cpp -o Tests.o

Main.o: src/main.cpp include/FileManager.h include/MetaManager.h include/QueryProcessor.h include/DatabaseManager.h include/RecordManager.h include/ColumnInfo.h include/Globals.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o Main.o

$(TARGET): FileManager.o MetaManager.o QueryProcessor.o RecordManager.o DatabaseManager.o Tests.o
	$(CXX) $(CXXFLAGS) FileManager.o MetaManager.o QueryProcessor.o RecordManager.o DatabaseManager.o Tests.o -o $(TARGET)

$(MAIN_TARGET): FileManager.o MetaManager.o QueryProcessor.o RecordManager.o DatabaseManager.o Main.o
	$(CXX) $(CXXFLAGS) FileManager.o MetaManager.o QueryProcessor.o RecordManager.o DatabaseManager.o Main.o -o $(MAIN_TARGET)

clean:
	rm -f *.o $(TARGET) $(MAIN_TARGET)
