#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
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
        std::function<void()> task;
        std::mutex taskMutex;
        Semaphore readySem{0};
        bool available = true;
    };

    size_t numThreads;
    std::vector<Worker> wts;

    std::thread dt;

    std::queue<std::function<void(void)>> taskQueue;
    std::mutex mutex;
    std::condition_variable allDone;

    std::queue<size_t> availableWorkersQueue;
    std::mutex availableWorkersMutex;

    Semaphore tasksAvailable{0};
    Semaphore workersAvailable{0};

    std::atomic<int> tasksInProgress{0};
    std::atomic<bool> shutdown{false};
    std::atomic<bool> acceptingTasks{true};
};

#endif





