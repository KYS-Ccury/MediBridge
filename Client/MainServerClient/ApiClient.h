// =====================================================
// ApiClient — 카테고리별 클라이언트의 조립자 (Facade)
// =====================================================
// 사용 예:
//   ApiClient client("http://localhost:8001");
//   client.auth().login("user@a.com", "pw", [](auto resp, int s) { ... });
//   client.pill().identify(req_id, "", true, callback);
//   client.history().record("201801234", 1, "메모", callback);
//
// 토큰 관리는 set_access_token() 한 번 호출하면 모든 카테고리에 전파.
// 시그널(token_expired, network_error)도 한 곳에서 수신.
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "AuthApiClient.h"
#include "PillApiClient.h"
#include "HistoryApiClient.h"
#include "ReportApiClient.h"
#include "MediaApiClient.h"
#include "SpeechApiClient.h"
#include "MonitoringApiClient.h"

namespace medibridge::network {

class ApiClientCommon;

class ApiClient : public QObject
{
    Q_OBJECT
public:
    explicit ApiClient(const QString& base_url, QObject* parent = nullptr);
    ~ApiClient() override;

    // ----- 카테고리 접근자 -----
    AuthApiClient&       auth()       { return auth_; }
    PillApiClient&       pill()       { return pill_; }
    HistoryApiClient&    history()    { return history_; }
    ReportApiClient&     report()     { return report_; }
    MediaApiClient&      media()      { return media_; }
    SpeechApiClient&     speech()     { return speech_; }
    MonitoringApiClient& monitoring() { return monitoring_; }

    // ----- JWT (Common 위임) -----
    void set_access_token(const QString& token);
    void clear_access_token();
    QString access_token() const;
    bool is_authenticated() const;

signals:
    /// Common의 시그널을 외부에 노출 (전파)
    void token_expired();
    void network_error(const QString& reason);

private:
    std::shared_ptr<ApiClientCommon> common_;

    // 카테고리별 멤버 — 모두 같은 common_ 공유
    AuthApiClient       auth_;
    PillApiClient       pill_;
    HistoryApiClient    history_;
    ReportApiClient     report_;
    MediaApiClient      media_;
    SpeechApiClient     speech_;
    MonitoringApiClient monitoring_;
};

} // namespace medibridge::network
