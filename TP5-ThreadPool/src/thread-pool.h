// thread-pool.h
#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Semaphore.h"

using namespace std;

typedef struct worker {
    thread ts;
} worker_t;

class ThreadPool {
  public:
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();
    
  private:
    void workerLoop(); 

    vector<thread> workers;
    queue<function<void(void)>> taskQueue;

    mutex queueLock;
    condition_variable cv_task;
    condition_variable cv_wait;

    size_t numThreads;
    size_t workingThreads = 0;
    bool shuttingDown = false;

    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif
