// =====================================================
// WorkerPool — 무거운 작업 분리용 스레드 풀 (싱글톤)
// =====================================================
// 호출자는 submit(...) 한 줄로 백그라운드 작업 위임.
// 내부 구현은 QThreadPool + QtConcurrent::run 사용.
// 향후 std::thread, boost::asio::thread_pool 등으로 교체 가능 (호출부 영향 없음).
//
// 사용 예:
//   auto future = WorkerPool::instance().submit([data]() {
//       return heavy_image_decode(data);  // 무거운 CPU 작업
//   });
//   // 결과는 QFutureWatcher 또는 future.result() 로 수신
// =====================================================
#pragma once

#include <QObject>
#include <QtConcurrent>
#include <QFuture>
#include <functional>

namespace medibridge::threading {

/**
 * @brief 클라이언트 측 무거운 작업용 워커 스레드 풀.
 *
 * - 싱글톤 (메인에서 instance() 첫 호출 시 초기화)
 * - 메인 스레드 차단 회피용 — 이미지 디코딩·암호화 등 CPU 바운드 작업 위임
 * - I/O 바운드 작업(네트워크 등)은 QNetworkAccessManager의 비동기 사용 권장 (본 풀 사용 X)
 */
class WorkerPool : public QObject
{
    Q_OBJECT
public:
    /// 싱글톤 인스턴스 접근
    static WorkerPool& instance();

    /**
     * @brief 작업을 워커 스레드에 위임.
     *
     * @tparam F 호출 가능 객체 (람다, 함수 포인터 등)
     * @tparam Args F의 인자 타입들
     * @param func 백그라운드에서 실행할 함수
     * @param args func 의 인자
     * @return QFuture — future.result() 또는 QFutureWatcher 로 결과 수신
     */
    template <typename F, typename... Args>
    auto submit(F&& func, Args&&... args)
        -> QFuture<std::invoke_result_t<F, Args...>>
    {
        return QtConcurrent::run(thread_pool_,
                                 std::forward<F>(func),
                                 std::forward<Args>(args)...);
    }

    /// 풀의 최대 스레드 수 (런타임 조정용)
    void set_max_thread_count(int count);
    int max_thread_count() const;

    /// 현재 활성 스레드 수 (모니터링용)
    int active_thread_count() const;

    /**
     * @brief 풀 종료 — 모든 작업 완료 대기 후 스레드 해제.
     * 메인 종료 시 자동 호출되지만 명시 호출도 가능.
     */
    void shutdown();

    // 복사·이동 금지 (싱글톤)
    WorkerPool(const WorkerPool&) = delete;
    WorkerPool& operator=(const WorkerPool&) = delete;

private:
    WorkerPool();
    ~WorkerPool() override;

    QThreadPool* thread_pool_;   // QObject 자식 — 자동 정리
};

} // namespace medibridge::threading
