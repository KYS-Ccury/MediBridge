#include "Monitoring.h"
#include "../Monitoring/HealthChecker.h"
#include "../Monitoring/MetricsExporter.h"
#include "../Schemas/HealthSchema.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

void Monitoring::handle_health(const drogon::HttpRequestPtr& req,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. medibridge::monitoring::HealthChecker::instance().check() 호출
    //   2. status: ok / degraded / down 결과 → HealthResponse JSON
    //   3. 200 (ok) / 503 (degraded·down)
    //
    // 점검 항목 (MonitoringApi.md):
    //   - DB 연결 (SELECT 1)
    //   - InferenceServer /health 호출
    //   - 디스크 여유 공간
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void Monitoring::handle_metrics(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. medibridge::monitoring::MetricsExporter::instance().collect() 호출
    //   2. format=prometheus 면 텍스트 형식, 기본 JSON
    //   3. SystemMetrics + ProcessMetrics + ServiceSpecificMetrics 응답
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
