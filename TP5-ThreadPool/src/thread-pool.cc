#include "thread-pool.h"
#include <stdexcept>

ThreadPool::ThreadPool(size_t numThreads)
    : numThreads(numThreads), workingThreads(0), shuttingDown(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() { workerLoop(); });
    }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (!thunk) {
        throw std::invalid_argument("Cannot schedule empty task");
    }
    unique_lock<mutex> lock(queueLock);
    if (shuttingDown) {
        throw runtime_error("ThreadPool has been destroyed or is shutting down");
    }
    taskQueue.push(thunk);
    cv_task.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        function<void(void)> task;
        {
            unique_lock<mutex> lock(queueLock);
            cv_task.wait(lock, [this]() {
                return shuttingDown || !taskQueue.empty();
            });

            if (shuttingDown && taskQueue.empty()) return;

            task = taskQueue.front();
            taskQueue.pop();
            ++workingThreads;
        }

        if (task) {
            task(); 
        } else {
            throw std::runtime_error("Scheduled empty task executed");
        }

        {
            unique_lock<mutex> lock(queueLock);
            --workingThreads;
            if (taskQueue.empty() && workingThreads == 0) {
                cv_wait.notify_all();
            }
        }
    }
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(queueLock);
    cv_wait.wait(lock, [this]() {
        return taskQueue.empty() && workingThreads == 0;
    });
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex> lock(queueLock);
        shuttingDown = true;
        cv_task.notify_all();
    }
    for (thread &t : workers) {
        if (t.joinable()) t.join();
    }
}
