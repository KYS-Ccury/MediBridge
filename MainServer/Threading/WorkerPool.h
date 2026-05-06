// =====================================================
// WorkerPool — 무거운 CPU 작업 분리용 스레드 풀 (싱글톤)
// =====================================================
// Drogon 자체 워커 풀과 별개로, 명시적으로 무거운 작업을 던지는 곳.
// 사용 예:
//   auto future = WorkerPool::instance().submit([data]() {
//       return heavy_pdf_render(data);
//   });
//   auto pdf = future.get();   // 또는 std::async 패턴
// =====================================================
#pragma once

#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
#include <atomic>

namespace medibridge::threading {

class WorkerPool
{
public:
    static WorkerPool& instance();

    /// 풀 초기화 (Main에서 한 번)
    void init(int worker_count);

    /// 작업 제출 — std::future 로 결과 수신
    template <typename F, typename... Args>
    auto submit(F&& func, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...)
        );
        auto future = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return future;
    }

    void shutdown();

    int worker_count() const;
    size_t pending_count() const;

private:
    WorkerPool() = default;
    ~WorkerPool();
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
};

} // namespace medibridge::threading
