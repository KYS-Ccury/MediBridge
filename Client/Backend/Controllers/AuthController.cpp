#include "AuthController.h"
#include "ApiClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

namespace medibridge::controllers {

AuthController::AuthController(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

bool    AuthController::is_authenticated() const { return is_authenticated_; }
QString AuthController::current_user_id() const { return current_user_id_; }
QString AuthController::current_user_email() const { return current_user_email_; }
QString AuthController::current_user_name() const { return current_user_name_; }
bool    AuthController::is_loading() const { return is_loading_; }
QString AuthController::last_error() const { return last_error_; }

void AuthController::set_loading(bool loading)
{
    if (is_loading_ == loading) return;
    is_loading_ = loading;
    emit is_loading_changed();
}

void AuthController::set_error(const QString& error_code)
{
    last_error_ = error_code;
    emit last_error_changed();
}

// =====================================================
// 로그인
// =====================================================
void AuthController::login(const QString& email, const QString& password)
{
    if (is_loading_) return;
    set_loading(true);
    set_error("");

    api_client_->auth().login(email, password,
        [this, email](const QByteArray& response, int status_code) {
            set_loading(false);

            const QJsonDocument doc = QJsonDocument::fromJson(response);

            if (status_code != 200) {
                QString code = "INVALID_CREDENTIALS";
                if (status_code == 0) {
                    code = "NETWORK_ERROR";
                } else if (doc.isObject() && doc.object().contains("error")) {
                    code = doc.object().value("error").toObject().value("code").toString(code);
                }
                set_error(code);
                emit login_failed(last_error_);
                qWarning() << "[AuthController] 로그인 실패 status=" << status_code << "code=" << code;
                return;
            }

            // 응답 파싱 — access_token 은 AuthApiClient 가 이미 common 에 set 함
            if (doc.isObject()) {
                const auto user = doc.object().value("user").toObject();
                current_user_id_    = user.value("user_id").toString();
                current_user_email_ = user.value("email").toString(email);
                current_user_name_  = user.value("user_name").toString();
            } else {
                current_user_email_ = email;
            }
            is_authenticated_ = true;

            emit current_user_id_changed();
            emit current_user_email_changed();
            emit current_user_name_changed();
            emit is_authenticated_changed();
            emit login_succeeded();

            qInfo() << "[AuthController] 로그인 성공:" << current_user_email_
                    << "user_id=" << current_user_id_;
        });
}

// =====================================================
// 회원가입
// =====================================================
void AuthController::signup(const QString& email,
                            const QString& password,
                            const QString& user_name)
{
    if (is_loading_) return;
    set_loading(true);
    set_error("");

    api_client_->auth().signup(email, password, user_name,
        [this](const QByteArray& response, int status_code) {
            set_loading(false);

            if (status_code != 201) {
                // 에러 envelope 에서 code 추출
                QString code = "SIGNUP_FAILED";
                const auto doc = QJsonDocument::fromJson(response);
                if (status_code == 0) {
                    code = "NETWORK_ERROR";
                } else if (doc.isObject() && doc.object().contains("error")) {
                    code = doc.object().value("error").toObject()
                                       .value("code").toString(code);
                }
                set_error(code);
                emit signup_failed(last_error_);
                qWarning() << "[AuthController] 회원가입 실패 status=" << status_code
                           << "code=" << code;
                return;
            }

            emit signup_succeeded();
            qInfo() << "[AuthController] 회원가입 성공";
        });
}

// =====================================================
// 로그아웃
// =====================================================
void AuthController::logout()
{
    if (is_loading_) return;
    set_loading(true);

    api_client_->auth().logout(
        [this](const QByteArray& /*response*/, int /*status*/) {
            // 응답 무시 — 클라 측 토큰 즉시 삭제
            api_client_->clear_access_token();

            is_authenticated_ = false;
            current_user_id_.clear();
            current_user_email_.clear();
            current_user_name_.clear();

            set_loading(false);

            emit is_authenticated_changed();
            emit current_user_id_changed();
            emit current_user_email_changed();
            emit current_user_name_changed();
            emit logout_completed();

            qInfo() << "[AuthController] 로그아웃 완료";
        });
}

} // namespace medibridge::controllers
