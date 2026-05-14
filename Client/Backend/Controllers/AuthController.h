// =====================================================
// AuthController — 로그인·회원가입·로그아웃 (ViewModel)
// =====================================================
// QML이 직접 ApiClient 호출하지 않도록 본 컨트롤러가 어댑터 역할.
// 비즈니스 로직은 ApiClient 위임. 본 클래스는 UI 상태 관리만.
// =====================================================
#pragma once

#include <QObject>
#include <QString>

namespace medibridge::network { class ApiClient; }

namespace medibridge::controllers {

class AuthController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool is_authenticated READ is_authenticated NOTIFY is_authenticated_changed)
    Q_PROPERTY(QString current_user_id READ current_user_id NOTIFY current_user_id_changed)
    Q_PROPERTY(QString current_user_email READ current_user_email NOTIFY current_user_email_changed)
    Q_PROPERTY(QString current_user_name READ current_user_name NOTIFY current_user_name_changed)
    Q_PROPERTY(bool is_loading READ is_loading NOTIFY is_loading_changed)
    Q_PROPERTY(QString last_error READ last_error NOTIFY last_error_changed)

public:
    explicit AuthController(network::ApiClient* api_client, QObject* parent = nullptr);

    // 접근자
    bool is_authenticated() const;
    QString current_user_id() const;
    QString current_user_email() const;
    QString current_user_name() const;
    bool is_loading() const;
    QString last_error() const;

    // QML 호출 메소드
    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void signup(const QString& email,
                            const QString& password,
                            const QString& user_name);
    Q_INVOKABLE void logout();

    /// 앱 시작 시 1회 호출 — QSettings 에서 토큰·프로필 복원 (FR-C7-04 자동 로그인)
    /// 복원 성공 시 is_authenticated=true 가 되며 login_succeeded 시그널이 발신됨.
    /// 토큰이 만료된 경우 첫 API 호출이 401 → token_expired 시그널 →
    /// on_token_expired() 슬롯이 세션을 정리한다.
    Q_INVOKABLE void restore_session();

signals:
    // 상태 변경 NOTIFY
    void is_authenticated_changed();
    void current_user_id_changed();
    void current_user_email_changed();
    void current_user_name_changed();
    void is_loading_changed();
    void last_error_changed();

    // 결과 시그널 (QML Connections로 수신)
    void login_succeeded();
    void login_failed(const QString& error_code);
    void signup_succeeded();
    void signup_failed(const QString& error_code);
    void logout_completed();
    /// 저장된 토큰이 만료되어 자동 로그아웃됨
    void session_expired();

public slots:
    /// ApiClient::token_expired 수신 — 저장 세션 즉시 폐기
    void on_token_expired();

private:
    void set_loading(bool loading);
    void set_error(const QString& error_code);
    void persist_session(const QString& token);
    void clear_persisted_session();

    network::ApiClient* api_client_;
    bool is_authenticated_ = false;
    QString current_user_id_;
    QString current_user_email_;
    QString current_user_name_;
    bool is_loading_ = false;
    QString last_error_;
};

} // namespace medibridge::controllers
