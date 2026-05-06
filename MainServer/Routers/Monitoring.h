// =====================================================
// Monitoring Router — /health, /metrics (MonitoringApi.md)
// =====================================================
// 인증 불필요 — 외부 모니터링 도구 접근 허용.
// /v1/ prefix 없음 (운영 표준 관례).
// =====================================================
#pragma once

#include <drogon/HttpController.h>

namespace medibridge::routers {

class Monitoring : public drogon::HttpController<Monitoring>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Monitoring::handle_health,  "/health",  drogon::Get);
    ADD_METHOD_TO(Monitoring::handle_metrics, "/metrics", drogon::Get);
    METHOD_LIST_END

    void handle_health(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_metrics(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
