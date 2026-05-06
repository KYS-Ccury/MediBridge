// =====================================================
// ResourceMonitor — 클라 PC 자체 자원 측정 (콘솔 로그)
// =====================================================
// CPU·메모리·디스크 사용률을 주기적으로 측정해 콘솔에 출력.
// UI 미연동 — 별도 모니터링 시스템에서 로그·metrics 수집 가정.
//
// Windows API (PDH 라이브러리) 또는 Qt + WMI 활용.
// =====================================================
#pragma once

#include <QObject>
#include <QTimer>

namespace medibridge::monitoring {

/**
 * @brief 클라이언트 PC 자체 자원 측정·로깅.
 *
 * 메인 스레드 소유. 주기적으로 측정·콘솔 로그.
 */
class ResourceMonitor : public QObject
{
    Q_OBJECT
public:
    /**
     * @param interval_ms 측정 주기 (ms)
     * @param parent QObject 부모
     */
    explicit ResourceMonitor(int interval_ms, QObject* parent = nullptr);

    /// 주기 측정 시작
    void start();
    /// 주기 측정 정지
    void stop();
    /// 즉시 1회 측정·로그
    void measure_now();

    // 최근 측정값 (Monitoring API /metrics 응답 작성에 사용)
    double last_cpu_percent() const;
    double last_memory_percent() const;
    double last_disk_percent() const;

signals:
    /// 측정 1회 완료 시 발신 (상위 모듈이 metrics 응답에 활용 가능)
    void measured(double cpu_percent, double memory_percent, double disk_percent);

private:
    /// Windows API로 CPU 사용률 측정
    double read_cpu_percent();
    /// 메모리 사용률 측정
    double read_memory_percent();
    /// 디스크 사용률 측정 (시스템 드라이브)
    double read_disk_percent();

    QTimer timer_;
    int interval_ms_;
    double last_cpu_percent_;
    double last_memory_percent_;
    double last_disk_percent_;
};

} // namespace medibridge::monitoring
