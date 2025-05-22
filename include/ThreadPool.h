#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "config.h"

class ThreadPool {
public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());

    ~ThreadPool();

    void submit(std::function<void()> job);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> running;
};

#endif