#include "../include/Utils/ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) {
    running = true;
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() {
            while (running) {
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