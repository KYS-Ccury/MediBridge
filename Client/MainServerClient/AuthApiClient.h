// =====================================================
// AuthApiClient — /v1/auth/* 호출 (AuthApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class AuthApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit AuthApiClient(std::shared_ptr<ApiClientCommon> common,
                           QObject* parent = nullptr);

    /// POST /v1/auth/signup
    void signup(const QString& email,
                const QString& password,
                const QString& user_name,
                JsonCallback callback);

    /// POST /v1/auth/login — 응답 성공 시 토큰 자동 저장
    void login(const QString& email,
               const QString& password,
               JsonCallback callback);

    /// POST /v1/auth/logout — 응답 성공 시 토큰 자동 삭제
    void logout(JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
