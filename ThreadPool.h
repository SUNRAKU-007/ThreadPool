#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>
#include <atomic>
#include <iostream>

// ============================================================
// UPGRADED THREAD POOL — All README challenges solved
//
// NEW vs original:
//  1. size()              — how many worker threads exist
//  2. completedTasks()    — how many tasks finished
//  3. Thread logging      — see which worker picks what task
//  4. Priority queue      — high priority tasks run first
//                           use submitWithPriority(p, fn, args...)
//  5. Bounded queue       — submit() blocks when queue is full
// ============================================================

struct Task {
    int priority;
    std::function<void()> fn;

    // MAX-heap: higher priority number runs first
    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

class ThreadPool {
public:
    // numThreads   : worker thread count
    // maxQueueSize : 0 = unlimited, >0 = blocks submit() when full
    // enableLogging: print which worker picks up each task
    explicit ThreadPool(size_t numThreads,
                        size_t maxQueueSize = 0,
                        bool enableLogging = false);
    ~ThreadPool();

    // Normal submit — priority 0 (FIFO among equal-priority tasks)
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    // Priority submit — higher number runs before lower
    template<class F, class... Args>
    auto submitWithPriority(int priority, F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    size_t size()           const { return workers.size(); }
    size_t pendingTasks();
    size_t completedTasks() const { return completed.load(); }
    void   setLogging(bool on)    { logging = on; }

private:
    void workerLoop(int workerId);

    template<class F, class... Args>
    auto submitImpl(int priority, F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    std::vector<std::thread>       workers;
    std::priority_queue<Task>      tasks;
    std::mutex                     queueMutex;
    std::condition_variable        taskAvailable;
    std::condition_variable        slotAvailable;
    bool                           stop;
    size_t                         maxQueueSize;
    std::atomic<size_t>            completed;
    bool                           logging;
};

// ── submitImpl (shared logic) ────────────────────────────────
template<class F, class... Args>
auto ThreadPool::submitImpl(int priority, F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using R = typename std::invoke_result<F, Args...>::type;
    auto taskPtr = std::make_shared<std::packaged_task<R()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    std::future<R> result = taskPtr->get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) throw std::runtime_error("submit() on stopped pool");
        if (maxQueueSize > 0) {
            slotAvailable.wait(lock, [this] {
                return stop || tasks.size() < maxQueueSize;
            });
        }
        if (stop) throw std::runtime_error("submit() on stopped pool");
        tasks.push({ priority, [taskPtr]() { (*taskPtr)(); } });
    }
    taskAvailable.notify_one();
    return result;
}

template<class F, class... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    return submitImpl(0, std::forward<F>(f), std::forward<Args>(args)...);
}

template<class F, class... Args>
auto ThreadPool::submitWithPriority(int priority, F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    return submitImpl(priority, std::forward<F>(f), std::forward<Args>(args)...);
}

#endif
