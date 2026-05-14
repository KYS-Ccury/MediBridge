#include "AuthController.h"
#include "ApiClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QSettings>

namespace medibridge::controllers {

namespace {
constexpr auto kKeyAccessToken = "auth/access_token";
constexpr auto kKeyUserId      = "auth/user_id";
constexpr auto kKeyEmail       = "auth/email";
constexpr auto kKeyUserName    = "auth/user_name";
}  // namespace

AuthController::AuthController(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
    // 토큰 만료 시그널 ↔ 자동 로그아웃 연동
    connect(api_client_, &network::ApiClient::token_expired,
            this, &AuthController::on_token_expired);
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
            QString token;
            if (doc.isObject()) {
                const auto obj  = doc.object();
                const auto user = obj.value("user").toObject();
                current_user_id_    = user.value("user_id").toString();
                current_user_email_ = user.value("email").toString(email);
                current_user_name_  = user.value("user_name").toString();
                token = obj.value("access_token").toString();
            } else {
                current_user_email_ = email;
            }
            is_authenticated_ = true;

            // 자동 로그인 영속화 (FR-C7-04)
            persist_session(token);

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
            clear_persisted_session();

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

// =====================================================
// 자동 로그인 — QSettings 복원 (FR-C7-04)
// =====================================================
void AuthController::restore_session()
{
    QSettings s;
    const QString token = s.value(kKeyAccessToken).toString();
    if (token.isEmpty()) {
        qInfo() << "[AuthController] 저장된 세션 없음 — 로그인 화면 유지";
        return;
    }

    current_user_id_    = s.value(kKeyUserId).toString();
    current_user_email_ = s.value(kKeyEmail).toString();
    current_user_name_  = s.value(kKeyUserName).toString();

    api_client_->set_access_token(token);
    is_authenticated_ = true;

    emit current_user_id_changed();
    emit current_user_email_changed();
    emit current_user_name_changed();
    emit is_authenticated_changed();
    emit login_succeeded();

    qInfo() << "[AuthController] 세션 복원 — email=" << current_user_email_
            << "(만료 시 다음 API 호출이 401 → on_token_expired)";
}

// =====================================================
// 토큰 만료 — ApiClient::token_expired 수신
// =====================================================
void AuthController::on_token_expired()
{
    if (!is_authenticated_) return;     // 이미 로그아웃 상태면 무시
    qWarning() << "[AuthController] 토큰 만료 — 세션 자동 폐기";

    api_client_->clear_access_token();
    clear_persisted_session();

    is_authenticated_ = false;
    current_user_id_.clear();
    current_user_email_.clear();
    current_user_name_.clear();

    emit is_authenticated_changed();
    emit current_user_id_changed();
    emit current_user_email_changed();
    emit current_user_name_changed();
    emit session_expired();
    emit logout_completed();    // Main.qml 라우터가 LoginPage 로 복귀
}

void AuthController::persist_session(const QString& token)
{
    QSettings s;
    s.setValue(kKeyAccessToken, token);
    s.setValue(kKeyUserId,      current_user_id_);
    s.setValue(kKeyEmail,       current_user_email_);
    s.setValue(kKeyUserName,    current_user_name_);
}

void AuthController::clear_persisted_session()
{
    QSettings s;
    s.remove(kKeyAccessToken);
    s.remove(kKeyUserId);
    s.remove(kKeyEmail);
    s.remove(kKeyUserName);
}

} // namespace medibridge::controllers
