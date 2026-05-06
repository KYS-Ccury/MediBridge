// =====================================================
// HealthChecker 구현 — 골격
// =====================================================
#include "HealthChecker.h"
#include "../MainServerClient/ApiClient.h"

#include <QLoggingCategory>
#include <QElapsedTimer>

namespace medibridge::monitoring {

HealthChecker::HealthChecker(network::ApiClient* api_client,
                             int interval_ms,
                             QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
    , interval_ms_(interval_ms)
    , last_reachable_(false)
    , last_latency_ms_(-1)
{
    timer_.setInterval(interval_ms_);
    connect(&timer_, &QTimer::timeout, this, &HealthChecker::check_now);
}

void HealthChecker::start()
{
    timer_.start();
    qInfo() << "[HealthChecker] 시작됨 — 주기:" << interval_ms_ << "ms";
    check_now();   // 첫 폴링 즉시
}

void HealthChecker::stop()
{
    timer_.stop();
    qInfo() << "[HealthChecker] 정지됨";
}

void HealthChecker::check_now()
{
    if (!api_client_) {
        return;
    }

    // TODO (영역 C 분담):
    //   1. QElapsedTimer 시작
    //   2. api_client_->check_health(callback) 호출
    //   3. 콜백에서 status_code 확인 (200 = ok, 503 = degraded, 0 = 네트워크 실패)
    //   4. 응답 시간 계산 → emit checked()
    //   5. 상태 전이(reachable ↔ unreachable) 감지 → 해당 시그널 emit
    //
    // 본 골격 단계에선 호출만 하고 콜백에서 로그.

    qInfo() << "[HealthChecker] /health 폴링 — TODO 구현";

    api_client_->check_health(
        [this](const QByteArray& response, int status_code) {
            const bool was_reachable = last_reachable_;
            last_reachable_ = (status_code == 200);
            last_latency_ms_ = -1;   // TODO: 측정값 채우기

            qInfo().nospace()
                << "[HealthChecker] MainServer status=" << status_code
                << " body=" << response.left(200);

            // 상태 전이 감지
            if (!was_reachable && last_reachable_) {
                emit main_server_recovered();
                qInfo() << "[HealthChecker] MainServer 복구됨";
            } else if (was_reachable && !last_reachable_) {
                emit main_server_unreachable();
                qWarning() << "[HealthChecker] MainServer 단절 감지";
            }

            emit checked(last_reachable_, last_latency_ms_);
        });
}

bool HealthChecker::last_main_server_reachable() const
{
    return last_reachable_;
}

int HealthChecker::last_main_server_latency_ms() const
{
    return last_latency_ms_;
}

} // namespace medibridge::monitoring
