# Thread Pool — C++17

I built this to deeply understand how multi-threading and 
synchronization work at a low level.

## Why I built this
After learning about concurrency in my OS course at NIT Srinagar,
I wanted to go beyond theory and implement a production-style 
thread pool from scratch.

## Features
- Fixed-size worker pool with graceful shutdown
- Priority queue — high priority tasks run first
- Bounded queue — submit() blocks when full
- Thread logging — prints which Worker ID picks up each task
- Returns std::future so callers get results asynchronously

## Results on my machine (12-core)
Sequential: 98.6 ms
Thread Pool (12 threads): 7.5 ms
Speedup: 13x

## How to run
g++ -std=c++17 main.cpp ThreadPool.cpp -o demo -pthread
./demo.exe

## What I learned
- Why condition_variable is needed over busy-waiting
- How deadlocks happen on shutdown and how to fix them
- How std::future and std::packaged_task work together
- Why bounded queues matter in production
