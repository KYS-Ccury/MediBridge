// =====================================================
// WorkerPool 구현 — 싱글톤 + QThreadPool 래핑
// =====================================================
#include "WorkerPool.h"

#include <QThread>
#include <QLoggingCategory>

namespace medibridge::threading {

// 기본 워커 스레드 수 = 시스템 코어 수와 일치
constexpr int DEFAULT_WORKER_COUNT = -1;   // -1 = QThread::idealThreadCount() 사용

// =====================================================
// 싱글톤 인스턴스
// =====================================================
WorkerPool& WorkerPool::instance()
{
    static WorkerPool instance;
    return instance;
}

// =====================================================
// 생성자 / 소멸자
// =====================================================
WorkerPool::WorkerPool()
    : QObject(nullptr)
    , thread_pool_(new QThreadPool(this))
{
    const int default_count = (DEFAULT_WORKER_COUNT > 0)
        ? DEFAULT_WORKER_COUNT
        : QThread::idealThreadCount();

    thread_pool_->setMaxThreadCount(default_count);

    qInfo() << "[WorkerPool] 초기화됨 — 최대 스레드 수:"
            << thread_pool_->maxThreadCount();
}

WorkerPool::~WorkerPool()
{
    shutdown();
}

// =====================================================
// 풀 설정
// =====================================================
void WorkerPool::set_max_thread_count(int count)
{
    if (count <= 0) {
        qWarning() << "[WorkerPool] 잘못된 스레드 수:" << count
                   << "→ 기본값 유지";
        return;
    }
    thread_pool_->setMaxThreadCount(count);
    qInfo() << "[WorkerPool] 최대 스레드 수 변경:" << count;
}

int WorkerPool::max_thread_count() const
{
    return thread_pool_->maxThreadCount();
}

int WorkerPool::active_thread_count() const
{
    return thread_pool_->activeThreadCount();
}

// =====================================================
// 종료
// =====================================================
void WorkerPool::shutdown()
{
    if (!thread_pool_) {
        return;
    }
    qInfo() << "[WorkerPool] 종료 중 — 활성 스레드:"
            << thread_pool_->activeThreadCount();
    thread_pool_->waitForDone();
    qInfo() << "[WorkerPool] 모든 작업 완료";
}

} // namespace medibridge::threading
