#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>

// A deliberately slow-ish function to simulate "real work"
// (e.g. processing a request, doing some computation).
long long slowSquareSum(int start, int end) {
    long long sum = 0;
    for (int i = start; i < end; ++i) {
        sum += static_cast<long long>(i) * i;
    }
    return sum;
}

void demoBasicUsage() {
    std::cout << "\n=== Demo 1: Basic submit + get result ===\n";

    ThreadPool pool(4);

    auto future1 = pool.submit(slowSquareSum, 0, 1000000);
    auto future2 = pool.submit(slowSquareSum, 1000000, 2000000);

    std::cout << "Tasks submitted, doing other work while they run...\n";

    // .get() blocks until that specific task is done
    std::cout << "Result 1: " << future1.get() << "\n";
    std::cout << "Result 2: " << future2.get() << "\n";
}

void demoManyTasks() {
    std::cout << "\n=== Demo 2: Submitting 20 tasks to a 4-thread pool ===\n";

    ThreadPool pool(4);
    std::vector<std::future<long long>> results;

    for (int i = 0; i < 20; ++i) {
        results.push_back(pool.submit(slowSquareSum, i * 100000, (i + 1) * 100000));
    }

    long long total = 0;
    for (auto& fut : results) {
        total += fut.get();
    }

    std::cout << "Total sum across all tasks: " << total << "\n";
}

void benchmarkSequentialVsPooled() {
    std::cout << "\n=== Demo 3: Sequential vs Thread Pool benchmark ===\n";

    const int numChunks = 100;
    const int chunkSize = 200000;

    // --- Sequential ---
    auto startSeq = std::chrono::high_resolution_clock::now();
    long long seqTotal = 0;
    for (int i = 0; i < numChunks; ++i) {
        seqTotal += slowSquareSum(i * chunkSize, (i + 1) * chunkSize);
    }
    auto endSeq = std::chrono::high_resolution_clock::now();
    double seqMs = std::chrono::duration<double, std::milli>(endSeq - startSeq).count();

    // --- Pooled (try changing the thread count below!) ---
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4; // fallback if undetectable

    auto startPool = std::chrono::high_resolution_clock::now();
    {
        ThreadPool pool(numThreads);
        std::vector<std::future<long long>> futures;
        for (int i = 0; i < numChunks; ++i) {
            futures.push_back(pool.submit(slowSquareSum, i * chunkSize, (i + 1) * chunkSize));
        }
        long long poolTotal = 0;
        for (auto& f : futures) poolTotal += f.get();
    }
    auto endPool = std::chrono::high_resolution_clock::now();
    double poolMs = std::chrono::duration<double, std::milli>(endPool - startPool).count();

    std::cout << "Detected " << numThreads << " hardware threads\n";
    std::cout << "Sequential time: " << seqMs << " ms (result: " << seqTotal << ")\n";
    std::cout << "Pooled time:     " << poolMs << " ms\n";
    std::cout << "Speedup:         " << (seqMs / poolMs) << "x\n";
    std::cout << "(Note: real speedup needs >1 CPU core. This sandbox only has "
                  "1, so run this on your own machine to see it properly.)\n";
}

int main() {
    demoBasicUsage();
    demoManyTasks();
    benchmarkSequentialVsPooled();
    return 0;
}
