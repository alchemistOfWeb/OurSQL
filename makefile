CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -I./include
LDFLAGS =

TARGET = run_tests

all: $(TARGET)

FileManager.o: src/FileManager.cpp include/FileManager.h
	$(CXX) $(CXXFLAGS) -c src/FileManager.cpp -o FileManager.o

RecordManager.o: src/RecordManager.cpp include/RecordManager.h
	$(CXX) $(CXXFLAGS) -c src/RecordManager.cpp -o RecordManager.o

MetaManager.o: src/MetaManager.cpp include/MetaManager.h
	$(CXX) $(CXXFLAGS) -c src/MetaManager.cpp -o MetaManager.o

QueryProcessor.o: src/QueryProcessor.cpp include/QueryProcessor.h
	$(CXX) $(CXXFLAGS) -c src/QueryProcessor.cpp -o QueryProcessor.o

Tests.o: tests/Tests.cpp include/FileManager.h include/MetaManager.h include/QueryProcessor.h include/RecordManager.h include/ColumnInfo.h include/Globals.h
	$(CXX) $(CXXFLAGS) -c tests/Tests.cpp -o Tests.o

$(TARGET): FileManager.o MetaManager.o QueryProcessor.o RecordManager.o Tests.o
	$(CXX) $(CXXFLAGS) FileManager.o MetaManager.o QueryProcessor.o RecordManager.o Tests.o -o $(TARGET)

clean:
	rm -f *.o $(TARGET)