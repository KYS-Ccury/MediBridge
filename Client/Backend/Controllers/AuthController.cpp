#include "AuthController.h"
#include "../../MainServerClient/ApiClient.h"

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

            if (status_code != 200) {
                // TODO: error_code 파싱 (response JSON의 error.code)
                set_error("INVALID_CREDENTIALS");
                emit login_failed(last_error_);
                return;
            }

            // TODO: JSON 파싱 → access_token, user 정보 추출
            // QJsonDocument doc = QJsonDocument::fromJson(response);
            // QString token = doc["access_token"].toString();
            // api_client_->set_access_token(token);
            // current_user_email_ = doc["user"]["email"].toString();
            // ...

            current_user_email_ = email;   // 임시
            is_authenticated_ = true;

            emit current_user_email_changed();
            emit is_authenticated_changed();
            emit login_succeeded();

            qInfo() << "[AuthController] 로그인 성공:" << email;
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
                // TODO: error_code 파싱
                set_error("SIGNUP_FAILED");
                emit signup_failed(last_error_);
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
