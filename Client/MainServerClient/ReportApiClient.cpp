#include "ReportApiClient.h"
#include "ApiClientCommon.h"

namespace medibridge::network {

ReportApiClient::ReportApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void ReportApiClient::generate(const QString& from_date,
                               const QString& to_date,
                               const QString& format,
                               JsonCallback callback)
{
    const QString path = QString("/v1/report/generate?from_date=%1&to_date=%2&format=%3")
                             .arg(from_date, to_date, format);
    common_->send_request("GET", path, {}, "application/json", std::move(callback));
}

} // namespace medibridge::network
