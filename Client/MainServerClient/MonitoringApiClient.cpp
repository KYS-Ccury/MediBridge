#include "MonitoringApiClient.h"
#include "ApiClientCommon.h"

namespace medibridge::network {

MonitoringApiClient::MonitoringApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void MonitoringApiClient::check_health(JsonCallback callback)
{
    common_->send_request("GET", "/health", {}, "application/json", std::move(callback));
}

void MonitoringApiClient::check_metrics(JsonCallback callback)
{
    common_->send_request("GET", "/metrics", {}, "application/json", std::move(callback));
}

} // namespace medibridge::network
