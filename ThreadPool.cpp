#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        // Each worker is a thread running workerLoop().
        workers.emplace_back([this] { this->workerLoop(); });
    }
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            // unique_lock (not lock_guard) because condition_variable
            // needs to be able to unlock/relock it internally while waiting.
            std::unique_lock<std::mutex> lock(queueMutex);

            // Sleep until there's a task OR we've been told to stop.
            // The lambda guards against "spurious wakeups" (the OS waking
            // a thread for no reason) — we re-check the condition on wake.
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            // If we're stopping AND the queue is drained, exit the loop.
            if (stop && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        } // lock released here, BEFORE we run the task —
          // otherwise no other worker could touch the queue while
          // this task is running, defeating the point of a pool.

        task();
    }
}

size_t ThreadPool::pendingTasks() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size();
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }

    // Wake up EVERY worker (not just one) so they all notice `stop`
    // and can exit their loop.
    condition.notify_all();

    for (std::thread& worker : workers) {
        worker.join(); // wait for each thread to actually finish
    }
}
