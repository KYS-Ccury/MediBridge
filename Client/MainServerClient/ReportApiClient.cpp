#include "ReportApiClient.h"
#include "ApiClientCommon.h"

#include <QUrlQuery>

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
    // ⚠ 보안: QUrlQuery 로 자동 percent-encoding (수동 문자열 결합 ❌)
    QUrlQuery query;
    query.addQueryItem("from_date", from_date);
    query.addQueryItem("to_date", to_date);
    query.addQueryItem("format", format);
    const QString path = QStringLiteral("/v1/report/generate?") +
                         query.toString(QUrl::FullyEncoded);
    common_->send_request("GET", path, {}, "application/json", std::move(callback));
}

} // namespace medibridge::network
