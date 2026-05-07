#include "AuthApiClient.h"
#include "ApiClientCommon.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace medibridge::network {

AuthApiClient::AuthApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void AuthApiClient::signup(const QString& email,
                           const QString& password,
                           const QString& user_name,
                           JsonCallback callback)
{
    QJsonObject body{
        {"email", email},
        {"password", password},
        {"user_name", user_name}
    };
    common_->send_request("POST", "/v1/auth/signup",
                          QJsonDocument(body).toJson(QJsonDocument::Compact),
                          "application/json", std::move(callback));
}

void AuthApiClient::login(const QString& email,
                          const QString& password,
                          JsonCallback callback)
{
    QJsonObject body{{"email", email}, {"password", password}};
    auto common = common_;
    common_->send_request("POST", "/v1/auth/login",
        QJsonDocument(body).toJson(QJsonDocument::Compact),
        "application/json",
        [common, cb = std::move(callback)](const QByteArray& resp, int status) {
            if (status == 200) {
                const auto doc = QJsonDocument::fromJson(resp);
                if (doc.isObject()) {
                    const auto token = doc.object().value("access_token").toString();
                    if (!token.isEmpty()) {
                        common->set_access_token(token);
                    }
                }
            }
            if (cb) cb(resp, status);
        });
}

void AuthApiClient::logout(JsonCallback callback)
{
    auto common = common_;
    common_->send_request("POST", "/v1/auth/logout", {}, "application/json",
        [common, cb = std::move(callback)](const QByteArray& resp, int status) {
            // 응답 성공 시 토큰 자동 삭제
            if (status >= 200 && status < 300) {
                common->clear_access_token();
            }
            if (cb) cb(resp, status);
        });
}

} // namespace medibridge::network
