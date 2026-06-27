#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

long long slowSquareSum(int start, int end) {
    long long sum = 0;
    for (int i = start; i < end; ++i)
        sum += static_cast<long long>(i) * i;
    return sum;
}

// ── Challenge 1: size() ──────────────────────────────────────
void demoSize() {
    std::cout << "\n=== Challenge 1: size() ===\n";
    ThreadPool pool4(4);
    ThreadPool pool8(8);
    std::cout << "pool4 has " << pool4.size() << " worker threads\n";
    std::cout << "pool8 has " << pool8.size() << " worker threads\n";
}

// ── Challenge 2: Thread logging ──────────────────────────────
void demoLogging() {
    std::cout << "\n=== Challenge 2: Thread Logging ===\n";
    std::cout << "Watch 4 workers pick up 8 tasks:\n";
    ThreadPool pool(4, 0, true);  // logging ON
    std::vector<std::future<long long>> futures;
    for (int i = 0; i < 8; ++i)
        futures.push_back(pool.submit(slowSquareSum, i * 50000, (i+1) * 50000));
    for (auto& f : futures) f.get();
    std::cout << "Total completed: " << pool.completedTasks() << "\n";
}

// ── Challenge 3: Priority Queue ──────────────────────────────
void demoPriority() {
    std::cout << "\n=== Challenge 3: Priority Queue ===\n";
    std::cout << "Expected order: HIGH → MEDIUM → LOW\n";

    // 1 worker so we can see ordering clearly
    ThreadPool pool(1);

    // Block the worker first so tasks queue up
    auto blocker = pool.submit([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        return 0;
    });

    // Submit out of order — priority decides execution order
    auto low = pool.submitWithPriority(1, []() -> std::string {
        std::cout << "  >> LOW priority task running\n";
        return "low";
    });
    auto medium = pool.submitWithPriority(5, []() -> std::string {
        std::cout << "  >> MEDIUM priority task running\n";
        return "medium";
    });
    auto high = pool.submitWithPriority(10, []() -> std::string {
        std::cout << "  >> HIGH priority task running\n";
        return "high";
    });

    blocker.get();
    high.get(); medium.get(); low.get();
}

// ── Challenge 4: Bounded Queue ───────────────────────────────
void demoBoundedQueue() {
    std::cout << "\n=== Challenge 4: Bounded Queue ===\n";

    // max 2 tasks in queue, 2 workers
    ThreadPool pool(2, 2);

    auto f1 = pool.submit([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::cout << "  Task 1 done\n"; return 1;
    });
    auto f2 = pool.submit([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::cout << "  Task 2 done\n"; return 2;
    });

    std::cout << "Queue full (2/2). Task 3 submit() will BLOCK...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    auto f3 = pool.submit([]() -> int {
        std::cout << "  Task 3 done\n"; return 3;
    });

    double waited = std::chrono::duration<double,std::milli>(
        std::chrono::high_resolution_clock::now() - t0).count();
    std::cout << "Task 3 submitted after blocking " << waited << " ms\n";

    f1.get(); f2.get(); f3.get();
    std::cout << "All done. Total: " << pool.completedTasks() << " tasks\n";
}

// ── Benchmark ────────────────────────────────────────────────
void benchmark() {
    std::cout << "\n=== Benchmark: Sequential vs Thread Pool ===\n";
    const int N = 100, chunk = 200000;
    unsigned int nt = std::thread::hardware_concurrency();
    if (!nt) nt = 4;

    auto t0 = std::chrono::high_resolution_clock::now();
    long long seq = 0;
    for (int i = 0; i < N; ++i) seq += slowSquareSum(i*chunk, (i+1)*chunk);
    double seqMs = std::chrono::duration<double,std::milli>(
        std::chrono::high_resolution_clock::now()-t0).count();

    auto t1 = std::chrono::high_resolution_clock::now();
    {
        ThreadPool pool(nt);
        std::vector<std::future<long long>> futs;
        for (int i = 0; i < N; ++i)
            futs.push_back(pool.submit(slowSquareSum, i*chunk, (i+1)*chunk));
        for (auto& f : futs) f.get();
    }
    double poolMs = std::chrono::duration<double,std::milli>(
        std::chrono::high_resolution_clock::now()-t1).count();

    std::cout << "Threads:     " << nt       << "\n";
    std::cout << "Sequential:  " << seqMs    << " ms\n";
    std::cout << "Pooled:      " << poolMs   << " ms\n";
    std::cout << "Speedup:     " << (seqMs/poolMs) << "x\n";
}

int main() {
    demoSize();
    demoLogging();
    demoPriority();
    demoBoundedQueue();
    benchmark();
    return 0;
}
