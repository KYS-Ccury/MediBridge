#include "PillApiClient.h"
#include "ApiClientCommon.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace medibridge::network {

PillApiClient::PillApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void PillApiClient::identify(const QString& image_request_id,
                             const QString& utterance_request_id,
                             bool include_dur_check,
                             JsonCallback callback)
{
    QJsonObject body{
        {"image_request_id", image_request_id},
        {"include_dur_check", include_dur_check}
    };
    if (!utterance_request_id.isEmpty()) {
        body["utterance_request_id"] = utterance_request_id;
    }
    common_->send_request("POST", "/v1/pill/identify",
                          QJsonDocument(body).toJson(QJsonDocument::Compact),
                          "application/json", std::move(callback));
}

void PillApiClient::get_pool(bool include_inactive, JsonCallback callback)
{
    const QString path = include_inactive
        ? "/v1/pill/pool?include_inactive=true"
        : "/v1/pill/pool";
    common_->send_request("GET", path, {}, "application/json", std::move(callback));
}

void PillApiClient::add_to_pool(const QString& item_code,
                                const QString& reg_method,
                                JsonCallback callback)
{
    QJsonObject body{{"item_code", item_code}, {"reg_method", reg_method}};
    common_->send_request("POST", "/v1/pill/pool",
                          QJsonDocument(body).toJson(QJsonDocument::Compact),
                          "application/json", std::move(callback));
}

void PillApiClient::remove_from_pool(int pool_id, JsonCallback callback)
{
    common_->send_request("DELETE",
                          QString("/v1/pill/pool/%1").arg(pool_id),
                          {}, "application/json", std::move(callback));
}

void PillApiClient::reset_pool(JsonCallback callback)
{
    QList<QPair<QByteArray, QByteArray>> headers{{"X-Confirm-Reset", "true"}};
    common_->send_request("DELETE", "/v1/pill/pool/all",
                          {}, "application/json", std::move(callback), headers);
}

void PillApiClient::search_drug_name(const QString& utterance_text, JsonCallback callback)
{
    // 사용자가 입력한 약 이름을 utterance_text 로 전달 → 서버 LIKE 매칭
    QJsonObject body{
        {"utterance_text", utterance_text},
        {"round", 1}
    };
    common_->send_request("POST", "/v1/pill/onboarding/normalize",
                          QJsonDocument(body).toJson(QJsonDocument::Compact),
                          "application/json", std::move(callback));
}

} // namespace medibridge::network
