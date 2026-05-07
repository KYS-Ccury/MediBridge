// =====================================================
// Monitoring Router — /health, /metrics
// =====================================================
// 인증 불필요. DB ping 으로 health 점검, 기본 metrics 는 stub
// (MetricsExporter 가 채워지면 자동 활용).
// =====================================================
#include "Monitoring.h"
#include "../Schemas/HealthSchema.h"
#include "../Database/Connection.h"
#include "../Utils/TimeUtil.h"
#include "../Config.h"

#include <drogon/HttpResponse.h>
#include <chrono>

using medibridge::database::Connection;
using medibridge::Config;

namespace medibridge::routers {

static const auto kBootTime = std::chrono::steady_clock::now();

void Monitoring::handle_health(const drogon::HttpRequestPtr& /*req*/,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    schemas::HealthResponse h;
    h.service        = "main_server";
    h.version        = "0.1.0";
    h.checked_at     = utils::current_iso8601_utc();
    h.uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - kBootTime).count();

    const bool db_ok = Connection::instance().ping();
    if (db_ok) {
        h.status = "ok";
    } else {
        h.status = "degraded";
        h.reasons.push_back("db_ping_failed");
    }
    if (Config::instance().test_mode()) {
        h.reasons.push_back("test_mode_active");
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(h.to_json());
    resp->setStatusCode(db_ok ? drogon::k200OK : drogon::k503ServiceUnavailable);
    callback(resp);
}

void Monitoring::handle_metrics(const drogon::HttpRequestPtr& /*req*/,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    schemas::MetricsResponse m;
    m.service      = "main_server";
    m.collected_at = utils::current_iso8601_utc();

    // 본 단계: 더미 값. MetricsExporter 가 ResourceMonitor 와 연결되면 실값 채움.
    m.system.cpu_percent     = 0;
    m.system.memory_used_mb  = 0;
    m.system.memory_total_mb = 0;
    m.system.memory_percent  = 0;
    m.system.disk_used_gb    = 0;
    m.system.disk_total_gb   = 0;
    m.system.disk_percent    = 0;
    m.system.uptime_seconds  = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - kBootTime).count();

    m.process.pid                   = 0;
    m.process.cpu_percent           = 0;
    m.process.memory_mb             = 0;
    m.process.thread_count          = 0;
    m.process.open_file_descriptors = 0;

    m.service_specific.active_connections       = 0;
    m.service_specific.request_rate_per_minute  = 0;
    m.service_specific.db_connection_pool_used  = 0;
    m.service_specific.db_connection_pool_total = Config::instance().db_pool_size();

    callback(drogon::HttpResponse::newHttpJsonResponse(m.to_json()));
}

} // namespace medibridge::routers
