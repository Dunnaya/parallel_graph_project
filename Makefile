# Makefile
CXX = g++
CXXFLAGS = -std=c++17 -pthread -O2 -Wall -Wextra
INCLUDES = -I./include
SRCDIR = src
SOURCES = $(SRCDIR)/parallel_graph.cpp
TARGET = parallel_graph

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

.PHONY: clean debug release
