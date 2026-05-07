#include "HistoryApiClient.h"
#include "ApiClientCommon.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace medibridge::network {

HistoryApiClient::HistoryApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void HistoryApiClient::record(const QString& item_code,
                              int quantity,
                              const QString& memo,
                              JsonCallback callback)
{
    QJsonObject body{
        {"item_code", item_code},
        {"quantity", quantity}
    };
    if (!memo.isEmpty()) {
        body["memo"] = memo;
    }
    common_->send_request("POST", "/v1/history/record",
                          QJsonDocument(body).toJson(QJsonDocument::Compact),
                          "application/json", std::move(callback));
}

void HistoryApiClient::list(const QString& from_date,
                            const QString& to_date,
                            int page,
                            int page_size,
                            JsonCallback callback)
{
    // ⚠ 보안: QUrlQuery 로 자동 percent-encoding (수동 문자열 결합 ❌).
    // 사용자 입력에 '&', '=', 한글, 공백 등 포함 시 안전 처리.
    QUrlQuery query;
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("page_size", QString::number(page_size));
    if (!from_date.isEmpty()) {
        query.addQueryItem("from_date", from_date);
    }
    if (!to_date.isEmpty()) {
        query.addQueryItem("to_date", to_date);
    }
    const QString path = QStringLiteral("/v1/history/list?") +
                         query.toString(QUrl::FullyEncoded);
    common_->send_request("GET", path, {}, "application/json", std::move(callback));
}

} // namespace medibridge::network
