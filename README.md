# Thread Pool Prototype (C++)

A working thread pool you can run, read, break, and extend. This is meant
to be a *learning* prototype — read the comments in `ThreadPool.h` and
`ThreadPool.cpp` first, they explain the "why" behind each line, not just
the "what."

## How to run it

```bash
make run
```

This compiles with `g++ -std=c++17 -pthread` and runs three demos:
1. Basic task submission with `.get()` on the result
2. Submitting 20 tasks to a 4-thread pool
3. A sequential-vs-pooled benchmark (run this on your own machine, not
   in a sandboxed/limited-core environment, to see a real speedup)

## File-by-file

- **ThreadPool.h** — the class definition + the template `submit()` method
  (templates have to live in headers in C++, that's not a style choice).
- **ThreadPool.cpp** — the actual worker loop, constructor, destructor.
  This is the file to study hardest — it's where all the synchronization
  logic lives.
- **main.cpp** — three demos showing how to use the pool.
- **Makefile** — `make run` to build+run, `make clean` to remove the binary.

## Things to try changing (in order of difficulty)

1. **Change the thread count** in `main.cpp` (`ThreadPool pool(4)` →
   try 1, 2, 8) and watch how the benchmark timing changes.

2. **Add a `size()` method** that returns how many worker threads exist
   (not pending tasks — that one already exists as `pendingTasks()`).

3. **Add logging** — print which thread ID picks up which task, so you
   can literally watch the scheduling happen:
   ```cpp
   std::cout << "Thread " << std::this_thread::get_id()
             << " picked up a task\n";
   ```

4. **Break it on purpose** — comment out the `std::lock_guard` in
   `pendingTasks()` and run with many threads hammering `submit()` at
   once. With luck (bad luck) you'll see a crash or garbage value. This
   is the best way to *feel* why locks matter, not just read about it.

5. **Add task priorities** — swap `std::queue<std::function<void()>>`
   for a `std::priority_queue`. You'll need to wrap tasks in a struct
   with a priority field and a custom comparator, since
   `std::function` itself isn't comparable.

6. **Stretch goal: bounded queue** — right now `submit()` never blocks,
   even if 10,000 tasks pile up. Add a max queue size and make
   `submit()` block (using another condition variable) when the queue
   is full. This is what real production thread pools do to avoid
   unbounded memory growth.

## Common bugs you might hit while extending this (and what they mean)

- **Deadlock on shutdown** — usually means you're calling `.join()`
  while still holding `queueMutex`. Lock scope matters; release the
  lock before blocking on something else.
- **Lost wakeup** — a worker waits forever even though a task was
  submitted. Usually means `notify_one()`/`notify_all()` was called
  while the lock was *not* held when it needed to be, or the predicate
  in `condition.wait()` is wrong.
- **Data race detected by ThreadSanitizer** — compile with
  `g++ -fsanitize=thread -std=c++17 -pthread ...` to catch races
  automatically. Worth doing once you start modifying the queue logic.
