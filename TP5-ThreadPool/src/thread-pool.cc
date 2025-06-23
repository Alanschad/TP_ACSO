#include "thread-pool.h"
#include <stdexcept>

ThreadPool::ThreadPool(size_t numThreads)
    : numThreads(numThreads), wts(numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        {
            std::lock_guard<std::mutex> lock(availableWorkersMutex);
            availableWorkersQueue.push(i);
        }
        workersAvailable.signal();
        wts[i].ts = std::thread([this, i] { worker(i); });
    }

    dt = std::thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const std::function<void(void)>& thunk) {
    if (!thunk)
        throw std::invalid_argument("Cannot schedule nullptr function");

    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!acceptingTasks.load() || shutdown.load())
            throw std::runtime_error("Cannot schedule: ThreadPool is shutting down");

        taskQueue.push(thunk);
        tasksInProgress++;
    }

    tasksAvailable.signal();
}

void ThreadPool::dispatcher() {
    while (true) {
        tasksAvailable.wait();

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (shutdown.load() && taskQueue.empty())
                break;
        }

        workersAvailable.wait();

        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (taskQueue.empty()) continue;
            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        size_t workerId;
        {
            std::lock_guard<std::mutex> lock(availableWorkersMutex);
            workerId = availableWorkersQueue.front();
            availableWorkersQueue.pop();
        }

        {
            std::lock_guard<std::mutex> lock(wts[workerId].taskMutex);
            wts[workerId].task = std::move(task);
            wts[workerId].available = false;
        }

        wts[workerId].readySem.signal();
    }
}

void ThreadPool::worker(size_t id) {
    while (true) {
        wts[id].readySem.wait();

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (shutdown.load() && tasksInProgress.load() == 0)
                break;
        }

        std::function<void()> localTask;
        {
            std::lock_guard<std::mutex> lock(wts[id].taskMutex);
            localTask = std::move(wts[id].task);
        }

        if (localTask) {
            try {
                localTask();
            } catch (...) {
                // Ignoramos excepciones de tareas
            }
        }

        {
            std::lock_guard<std::mutex> lock(wts[id].taskMutex);
            wts[id].available = true;
        }

        {
            std::lock_guard<std::mutex> lock(availableWorkersMutex);
            availableWorkersQueue.push(id);
        }

        workersAvailable.signal();

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (tasksInProgress.fetch_sub(1) == 1) {
                allDone.notify_all();
            }
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    allDone.wait(lock, [this] {
        return taskQueue.empty() && tasksInProgress.load() == 0;
    });
}

ThreadPool::~ThreadPool() {
    acceptingTasks.store(false);  

    wait();                        
    shutdown.store(true);        
    tasksAvailable.signal();      
    if (dt.joinable()) dt.join(); 

    for (auto& w : wts) {
        w.readySem.signal();     
        if (w.ts.joinable()) w.ts.join();
    }
}




