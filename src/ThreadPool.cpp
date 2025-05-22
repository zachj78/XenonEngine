#include "../include/ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) {
    running = true;
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    condition.wait(lock, [this]() {
                        return !tasks.empty() || !running;
                        });

                    if (!running && tasks.empty())
                        return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
            });
    }
}

void ThreadPool::submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.push(std::move(job));
    }
    condition.notify_one();
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }
    condition.notify_all();
    for (std::thread& t : workers)
        if (t.joinable())
            t.join();
}