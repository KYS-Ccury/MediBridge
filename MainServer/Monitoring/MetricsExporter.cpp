#include "MetricsExporter.h"
#include "ResourceMonitor.h"
#include "../Utils/TimeUtil.h"

namespace medibridge::monitoring {

MetricsExporter& MetricsExporter::instance()
{
    static MetricsExporter instance;
    return instance;
}

schemas::MetricsResponse MetricsExporter::collect_json()
{
    schemas::MetricsResponse resp;
    resp.service = "main_server";

    // ISO 8601 현재 시각 — thread-safe 헬퍼 사용
    resp.collected_at = utils::current_iso8601_utc();

    // ResourceMonitor 의 최근 측정값 사용
    ResourceMonitor::instance().measure();
    auto& rm = ResourceMonitor::instance();

    resp.system.cpu_percent      = rm.last_cpu_percent();
    resp.system.memory_used_mb   = rm.last_memory_used_mb();
    resp.system.memory_total_mb  = rm.last_memory_total_mb();
    resp.system.memory_percent   = rm.last_memory_percent();
    resp.system.disk_used_gb     = 0.0;          // TODO
    resp.system.disk_total_gb    = 0.0;          // TODO
    resp.system.disk_percent     = rm.last_disk_percent();
    resp.system.uptime_seconds   = rm.last_uptime_seconds();

    // TODO (영역 B 분담):
    //   - getpid(), /proc/self/status 파싱 → ProcessMetrics
    //   - Drogon DB 풀 사용 통계 → ServiceSpecificMetrics
    resp.process.pid                       = 0;
    resp.process.cpu_percent               = 0.0;
    resp.process.memory_mb                 = 0.0;
    resp.process.thread_count              = 0;
    resp.process.open_file_descriptors     = 0;

    resp.service_specific.active_connections        = 0;
    resp.service_specific.request_rate_per_minute   = 0;
    resp.service_specific.db_connection_pool_used   = 0;
    resp.service_specific.db_connection_pool_total  = 0;

    return resp;
}

std::string MetricsExporter::collect_prometheus()
{
    // TODO (확장): Prometheus 텍스트 형식
    //   # HELP cpu_percent ...
    //   # TYPE cpu_percent gauge
    //   cpu_percent{service="main_server"} 12.3
    return "# Prometheus format — TODO\n";
}

} // namespace medibridge::monitoring
