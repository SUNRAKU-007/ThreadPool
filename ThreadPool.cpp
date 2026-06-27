#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads, size_t maxQueueSize, bool enableLogging)
    : stop(false), maxQueueSize(maxQueueSize), completed(0), logging(enableLogging)
{
    for (size_t i = 0; i < numThreads; ++i) {
        // Pass worker ID so logging can identify each thread
        workers.emplace_back([this, i] { this->workerLoop(static_cast<int>(i)); });
    }
}

void ThreadPool::workerLoop(int workerId) {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            taskAvailable.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty()) return;

            // priority_queue gives us the highest priority task at .top()
            task = tasks.top();
            tasks.pop();

            // Tell submit() a slot just opened — it may be waiting
            if (maxQueueSize > 0) {
                slotAvailable.notify_one();
            }
        }

        // LOGGING: show which worker picked up this task
        if (logging) {
            std::lock_guard<std::mutex> coutLock(queueMutex);
            std::cout << "[Worker " << workerId
                      << " | Thread " << std::this_thread::get_id()
                      << " | Priority " << task.priority
                      << "] picked up a task\n";
        }

        task.fn();
        completed++;
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
    taskAvailable.notify_all();
    slotAvailable.notify_all();

    for (std::thread& w : workers) {
        w.join();
    }
}
