#include "WorkerPool.h"

#include <iostream>

namespace medibridge::threading {

WorkerPool& WorkerPool::instance()
{
    static WorkerPool instance;
    return instance;
}

WorkerPool::~WorkerPool()
{
    shutdown();
}

void WorkerPool::init(int worker_count)
{
    if (worker_count <= 0) {
        worker_count = std::thread::hardware_concurrency();
    }
    stop_ = false;
    for (int i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this] { worker_loop(); });
    }
    std::cout << "[WorkerPool] 초기화 — 워커 수: " << worker_count << std::endl;
}

void WorkerPool::worker_loop()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "[WorkerPool] 작업 예외: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[WorkerPool] 작업 예외 (unknown)" << std::endl;
        }
    }
}

void WorkerPool::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    std::cout << "[WorkerPool] 종료됨" << std::endl;
}

int WorkerPool::worker_count() const
{
    return static_cast<int>(workers_.size());
}

size_t WorkerPool::pending_count() const
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

} // namespace medibridge::threading
