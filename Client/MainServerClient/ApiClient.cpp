// =====================================================
// ApiClient 구현 — 골격 (TODO 주석으로 분담 작업 안내)
// =====================================================
#include "ApiClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QLoggingCategory>
#include <QUuid>

namespace medibridge::network {

// =====================================================
// 생성자 / 소멸자
// =====================================================
ApiClient::ApiClient(const QString& base_url, QObject* parent)
    : QObject(parent)
    , base_url_(base_url)
    , network_manager_(new QNetworkAccessManager(this))
{
    // base_url 끝에 슬래시가 있으면 제거 (path 결합 시 중복 방지)
    if (base_url_.endsWith('/')) {
        base_url_.chop(1);
    }
}

ApiClient::~ApiClient() = default;

// =====================================================
// JWT 토큰 관리
// =====================================================
void ApiClient::set_access_token(const QString& token)
{
    access_token_ = token;
    qInfo() << "[ApiClient] JWT 토큰 등록됨 (length:" << token.length() << ")";
}

void ApiClient::clear_access_token()
{
    access_token_.clear();
    qInfo() << "[ApiClient] JWT 토큰 삭제됨";
}

QString ApiClient::access_token() const
{
    return access_token_;
}

bool ApiClient::is_authenticated() const
{
    return !access_token_.isEmpty();
}

// =====================================================
// 공통 요청 빌더
// =====================================================
QNetworkRequest ApiClient::build_request(const QString& path,
                                         const QString& content_type) const
{
    const QUrl url(base_url_ + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, content_type);

    // JWT 자동 첨부
    if (!access_token_.isEmpty()) {
        request.setRawHeader("Authorization",
                             ("Bearer " + access_token_).toUtf8());
    }

    // 요청 추적용 X-Request-ID (UUID)
    request.setRawHeader("X-Request-ID",
                         QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());

    return request;
}

// =====================================================
// 공통 송신
// =====================================================
void ApiClient::send_request(const QString& method,
                             const QString& path,
                             const QByteArray& body,
                             const QString& content_type,
                             JsonCallback callback,
                             const QList<QPair<QByteArray, QByteArray>>& extra_headers)
{
    // TODO (영역 C 분담):
    //   1. build_request(path, content_type) 으로 요청 생성
    //   2. extra_headers 추가 첨부 (예: X-Confirm-Reset)
    //   3. method 별로 network_manager_->post/get/deleteResource() 호출
    //   4. QNetworkReply::finished 시그널 → 콜백 호출
    //   5. 401 감지 시 token_expired 시그널 emit
    //   6. 네트워크 에러 시 network_error 시그널 emit
    //
    // 참고: ApiOverview.md (공통 헤더, 표준 에러)
    Q_UNUSED(method);
    Q_UNUSED(path);
    Q_UNUSED(body);
    Q_UNUSED(content_type);
    Q_UNUSED(extra_headers);
    qInfo() << "[ApiClient] send_request — TODO 구현:" << method << path;
    if (callback) {
        callback(QByteArray("{\"todo\":true}"), 200);
    }
}

// =====================================================
// Auth API
// =====================================================
void ApiClient::signup(const QString& email,
                       const QString& password,
                       const QString& user_name,
                       JsonCallback callback)
{
    // TODO: SignupRequest JSON 생성 → POST /v1/auth/signup
    //   (Schemas/AuthSchema 와 매핑 — MainServer 측 정의와 일치 필요)
    QJsonObject body{
        {"email", email},
        {"password", password},
        {"user_name", user_name}
    };
    send_request("POST", "/v1/auth/signup",
                 QJsonDocument(body).toJson(QJsonDocument::Compact),
                 "application/json",
                 callback);
}

void ApiClient::login(const QString& email,
                      const QString& password,
                      JsonCallback callback)
{
    // TODO: 응답 성공 시 access_token을 자동 set_access_token 호출
    QJsonObject body{
        {"email", email},
        {"password", password}
    };
    send_request("POST", "/v1/auth/login",
                 QJsonDocument(body).toJson(QJsonDocument::Compact),
                 "application/json",
                 callback);
}

void ApiClient::logout(JsonCallback callback)
{
    // TODO: 응답 성공 시 clear_access_token 호출
    send_request("POST", "/v1/auth/logout", {}, "application/json", callback);
}

// =====================================================
// Media API
// =====================================================
void ApiClient::upload_image(const QByteArray& image_data,
                             const QString& mime_type,
                             const QString& intent_hint,
                             JsonCallback callback)
{
    // TODO (영역 C 분담):
    //   1. QHttpMultiPart 생성 → image part + intent_hint part
    //   2. network_manager_->post(request, multiPart) 호출
    //   3. 응답을 콜백으로 전달
    Q_UNUSED(image_data);
    Q_UNUSED(mime_type);
    Q_UNUSED(intent_hint);
    qInfo() << "[ApiClient] upload_image — TODO multipart 구현 ("
            << image_data.size() << "bytes," << mime_type << ")";
    if (callback) {
        callback(QByteArray("{\"todo\":true}"), 202);
    }
}

// =====================================================
// Speech API
// =====================================================
void ApiClient::send_utterance(const QString& text,
                               double stt_confidence,
                               const QString& context,
                               const QString& image_request_id,
                               JsonCallback callback)
{
    QJsonObject body{
        {"text", text},
        {"stt_confidence", stt_confidence},
        {"stt_engine", "galaxy_ai"},
        {"context", context.isEmpty() ? "daily_use" : context},
        {"language", "ko-KR"}
    };
    if (!image_request_id.isEmpty()) {
        body["image_request_id"] = image_request_id;
    }
    send_request("POST", "/v1/speech/utterance",
                 QJsonDocument(body).toJson(QJsonDocument::Compact),
                 "application/json",
                 callback);
}

// =====================================================
// Pill API (TODO 구현)
// =====================================================
void ApiClient::identify_pill(const QString& image_request_id,
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
    send_request("POST", "/v1/pill/identify",
                 QJsonDocument(body).toJson(QJsonDocument::Compact),
                 "application/json",
                 callback);
}

void ApiClient::get_pill_pool(bool include_inactive, JsonCallback callback)
{
    const QString path = include_inactive
        ? "/v1/pill/pool?include_inactive=true"
        : "/v1/pill/pool";
    send_request("GET", path, {}, "application/json", callback);
}

void ApiClient::add_pill_to_pool(const QString& item_code,
                                 const QString& reg_method,
                                 JsonCallback callback)
{
    QJsonObject body{
        {"item_code", item_code},
        {"reg_method", reg_method}
    };
    send_request("POST", "/v1/pill/pool",
                 QJsonDocument(body).toJson(QJsonDocument::Compact),
                 "application/json",
                 callback);
}

void ApiClient::remove_pill_from_pool(int pool_id, JsonCallback callback)
{
    send_request("DELETE", QString("/v1/pill/pool/%1").arg(pool_id),
                 {}, "application/json", callback);
}

void ApiClient::reset_pill_pool(JsonCallback callback)
{
    // 안전 가드 — X-Confirm-Reset 헤더 강제 첨부
    QList<QPair<QByteArray, QByteArray>> headers{
        {"X-Confirm-Reset", "true"}
    };
    send_request("DELETE", "/v1/pill/pool/all",
                 {}, "application/json", callback, headers);
}

// =====================================================
// History API
// =====================================================
void ApiClient::record_history(const QString& item_code,
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
    send_request("POST", "/v1/history/record",
                 QJsonDocument(body).toJson(QJsonDocument::Compact),
                 "application/json",
                 callback);
}

void ApiClient::list_history(const QString& from_date,
                             const QString& to_date,
                             int page,
                             int page_size,
                             JsonCallback callback)
{
    QString path = QString("/v1/history/list?page=%1&page_size=%2")
                       .arg(page).arg(page_size);
    if (!from_date.isEmpty()) {
        path += "&from_date=" + from_date;
    }
    if (!to_date.isEmpty()) {
        path += "&to_date=" + to_date;
    }
    send_request("GET", path, {}, "application/json", callback);
}

// =====================================================
// Report API
// =====================================================
void ApiClient::generate_report(const QString& from_date,
                                const QString& to_date,
                                const QString& format,
                                JsonCallback callback)
{
    const QString path = QString("/v1/report/generate?from_date=%1&to_date=%2&format=%3")
                             .arg(from_date, to_date, format);
    send_request("GET", path, {}, "application/json", callback);
}

// =====================================================
// Monitoring API
// =====================================================
void ApiClient::check_health(JsonCallback callback)
{
    send_request("GET", "/health", {}, "application/json", callback);
}

void ApiClient::check_metrics(JsonCallback callback)
{
    send_request("GET", "/metrics", {}, "application/json", callback);
}

} // namespace medibridge::network
