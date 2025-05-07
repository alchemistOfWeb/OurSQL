CXX = g++
CXXFLAGS = -std=c++20 -Wall -Iinclude

SRCS = src/main.cpp src/QueryManager.cpp src/MetaManager.cpp src/DataManager.cpp src/FileDispatcher.cpp
OBJS = $(SRCS:.cpp=.o)

all: oursql

oursql: $(OBJS)
	$(CXX) $(CXXFLAGS) -o oursql $(OBJS)

clean:
	rm -f oursql src/*.o