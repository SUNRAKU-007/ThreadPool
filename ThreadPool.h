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

// A simple, fixed-size thread pool.
//
// HOW IT WORKS (read this before reading the code):
// 1. The constructor spawns `numThreads` worker threads. Each one runs
//    `workerLoop()` forever, waiting for tasks to show up in the queue.
// 2. `submit()` puts a task into the queue and wakes ONE sleeping worker.
// 3. A worker wakes up, grabs the task off the queue, runs it, then goes
//    back to waiting.
// 4. The destructor sets a "stop" flag and wakes everyone up so they can
//    exit their loop cleanly instead of hanging forever.
//
// The tricky part is always synchronization: many threads touch the same
// queue, so every access to it must be protected by `queueMutex`.

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Submit a task and get back a future you can call .get() on to
    // retrieve the result (or block until the task finishes).
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    // Returns how many tasks are currently waiting (not yet started).
    size_t pendingTasks();

private:
    void workerLoop();

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// --- Template method must live in the header (C++ requirement) ---

template<class F, class... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using ReturnType = typename std::invoke_result<F, Args...>::type;

    // package_task wraps the function so we can attach a future to it
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (stop) {
            throw std::runtime_error("submit() called on a stopped ThreadPool");
        }

        // Wrap the packaged_task in a plain void() lambda so it fits in
        // our std::function<void()> queue.
        tasks.emplace([task]() { (*task)(); });
    }

    // Wake up exactly one worker — only one new task is available.
    condition.notify_one();
    return result;
}

#endif // THREAD_POOL_H
