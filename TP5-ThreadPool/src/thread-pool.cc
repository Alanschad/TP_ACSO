#include "thread-pool.h"
#include <stdexcept>

ThreadPool::ThreadPool(size_t numThreads) : numThreads(numThreads), wts(numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i].ts = std::thread([this, i] { worker(i); });
    }
    dt = std::thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const std::function<void(void)>& thunk) {
    if (!thunk) throw std::invalid_argument("Cannot schedule nullptr function");

    if (shutdown.load()) 
        throw std::runtime_error("ThreadPool already shut down");

    {
        std::unique_lock<std::mutex> lock(mutex);
        taskQueue.push(thunk);
        tasksInProgress++;
        cv.notify_one();
    }
}



void ThreadPool::dispatcher() {
    while (true) {
        std::function<void(void)> task;

        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return !taskQueue.empty() || shutdown.load(); });

            if (shutdown.load() && taskQueue.empty()) break;

            task = taskQueue.front();
            taskQueue.pop();
        }

        bool assigned = false;
        while (!assigned) {
            for (size_t i = 0; i < numThreads; ++i) {
                std::lock_guard<std::mutex> lock(wts[i].taskMutex);
                if (!wts[i].busy) {
                    wts[i].task = task;
                    wts[i].busy = true;
                    wts[i].readySem.signal();
                    assigned = true;
                    break;
                }
            }
            if (!assigned) std::this_thread::yield();
        }
    }
}

void ThreadPool::worker(size_t id) {
    while (true) {
        wts[id].readySem.wait();

        {
            std::unique_lock<std::mutex> lock(mutex);
            if (shutdown.load()) break;
        }

        std::function<void(void)> localTask;

        {
            std::lock_guard<std::mutex> lock(wts[id].taskMutex);
            localTask = std::move(wts[id].task);
        }

        if (localTask) {
            try {
                localTask();
            } catch (...) {
                
            }
        }

        {
            std::lock_guard<std::mutex> lock(wts[id].taskMutex);
            wts[id].busy = false;
        }

        {
            std::unique_lock<std::mutex> lock(mutex);
            tasksInProgress--;
            if (tasksInProgress == 0) {
                allDone.notify_all();
            }
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    allDone.wait(lock, [this] { return tasksInProgress == 0; });
}

ThreadPool::~ThreadPool() {
    wait();

    shutdown.store(true);

    {
        std::lock_guard<std::mutex> lock(mutex);
        cv.notify_all();
    }

    if (dt.joinable()) dt.join();

    for (auto& w : wts) {
        w.readySem.signal();
        if (w.ts.joinable()) w.ts.join();
    }
}

