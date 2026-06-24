CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread

TARGET = thread_pool_demo
SOURCES = main.cpp ThreadPool.cpp

all: $(TARGET)

$(TARGET): $(SOURCES) ThreadPool.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
