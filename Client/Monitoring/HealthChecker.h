// =====================================================
// HealthChecker — MainServer /health 주기 폴링 (콘솔 로그)
// =====================================================
// 본 클래스는 ApiClient::check_health 를 주기 호출하여 결과를 콘솔에 로그.
// UI 미연동 — 별도 모니터링 시스템 가정.
//
// 통신 단절 감지 시 자동 재시도 (Phase 3 안정화 단계 채움).
// =====================================================
#pragma once

#include <QObject>
#include <QTimer>

namespace medibridge::network { class ApiClient; }

namespace medibridge::monitoring {

class HealthChecker : public QObject
{
    Q_OBJECT
public:
    /**
     * @param api_client MainServer 호출 클라이언트 (외부 소유)
     * @param interval_ms 폴링 주기 (ms)
     * @param parent QObject 부모
     */
    explicit HealthChecker(network::ApiClient* api_client,
                           int interval_ms,
                           QObject* parent = nullptr);

    /// 주기 폴링 시작
    void start();
    /// 주기 폴링 정지
    void stop();
    /// 즉시 1회 폴링
    void check_now();

    // 최근 결과 (Monitoring API /metrics 응답 작성에 사용)
    bool last_main_server_reachable() const;
    int  last_main_server_latency_ms() const;

signals:
    /// 폴링 1회 완료 시 발신
    void checked(bool reachable, int latency_ms);

    /// 통신 단절 → 재연결 성공 전이 시 발신
    void main_server_recovered();
    /// 통신 정상 → 단절 전이 시 발신
    void main_server_unreachable();

private:
    network::ApiClient* api_client_;
    QTimer timer_;
    int interval_ms_;
    bool last_reachable_;
    int  last_latency_ms_;
};

} // namespace medibridge::monitoring
