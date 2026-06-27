CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread

all: demo

demo: main.cpp ThreadPool.cpp ThreadPool.h
	$(CXX) $(CXXFLAGS) main.cpp ThreadPool.cpp -o demo

run: demo
	./demo

clean:
	rm -f demo demo.exe

.PHONY: all run clean
