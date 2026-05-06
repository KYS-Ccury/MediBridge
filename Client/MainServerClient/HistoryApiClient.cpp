#include "HistoryApiClient.h"
#include "ApiClientCommon.h"

#include <QJsonDocument>
#include <QJsonObject>

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
    QString path = QString("/v1/history/list?page=%1&page_size=%2")
                       .arg(page).arg(page_size);
    if (!from_date.isEmpty()) path += "&from_date=" + from_date;
    if (!to_date.isEmpty())   path += "&to_date=" + to_date;
    common_->send_request("GET", path, {}, "application/json", std::move(callback));
}

} // namespace medibridge::network
