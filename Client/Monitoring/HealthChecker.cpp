// =====================================================
// HealthChecker 구현 — 골격
// =====================================================
#include "HealthChecker.h"
#include "ApiClient.h"

#include <QLoggingCategory>
#include <QElapsedTimer>

#include <memory>

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

    // 응답 시간 측정용 timer — shared_ptr 로 콜백 수명 길이 보장
    auto et = std::make_shared<QElapsedTimer>();
    et->start();

    api_client_->monitoring().check_health(
        [this, et](const QByteArray& response, int status_code) {
            const qint64 elapsed = et->elapsed();
            const bool was_reachable = last_reachable_;
            last_reachable_ = (status_code == 200);
            last_latency_ms_ = static_cast<int>(elapsed);

            qInfo().nospace()
                << "[HealthChecker] MainServer status=" << status_code
                << " latency=" << elapsed << "ms"
                << " body=" << response.left(120);

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
