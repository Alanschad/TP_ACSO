#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "Semaphore.h"  

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    void schedule(const std::function<void(void)>& thunk);
    void wait();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    void dispatcher();
    void worker(size_t id);

    struct Worker {
        std::thread ts;
        std::function<void(void)> task;
        std::mutex taskMutex;
        Semaphore readySem{0};
        bool busy = false;
    };

    size_t numThreads;
    std::vector<Worker> wts;

    std::queue<std::function<void(void)>> taskQueue;
    std::thread dt;

    std::mutex mutex;
    std::condition_variable cv;
    std::condition_variable allDone;

    std::atomic<bool> shutdown{false};
    std::atomic<int> tasksInProgress{0};
};

#endif

