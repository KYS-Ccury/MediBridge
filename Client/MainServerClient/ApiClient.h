// =====================================================
// ApiClient — MainServer REST API 호출 클라이언트
// =====================================================
// QNetworkAccessManager 기반 비동기 호출.
// JWT 토큰을 자동 헤더에 첨부 (로그인 후 set_access_token 호출).
//
// 모든 메소드는 메인 스레드에서 호출 가능 (네트워크 I/O는 비동기).
// 결과는 std::function 콜백으로 받음.
//
// 401 (토큰 만료) 감지 시 token_expired 시그널 발신 → UI에서 재로그인 유도.
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <functional>

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

namespace medibridge::network {

/**
 * @brief MainServer로 REST API 호출 클라이언트.
 *
 * 사용 예:
 *   ApiClient client("http://localhost:8001");
 *   client.login("user@a.com", "pw", [](auto resp, int status) {
 *       qInfo() << "로그인 응답:" << resp;
 *   });
 */
class ApiClient : public QObject
{
    Q_OBJECT
public:
    /// 응답 콜백 시그니처 — (응답 본문 raw, HTTP 상태 코드)
    using JsonCallback = std::function<void(const QByteArray& response_body, int status_code)>;

    /**
     * @param base_url 메인서버 베이스 URL (예: "http://localhost:8001")
     * @param parent QObject 부모
     */
    explicit ApiClient(const QString& base_url, QObject* parent = nullptr);
    ~ApiClient() override;

    // ----- JWT 토큰 관리 -----
    void set_access_token(const QString& token);
    void clear_access_token();
    QString access_token() const;
    bool is_authenticated() const;

    // ----- Auth API (AuthApi.md) -----
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

    // ----- Media API (MediaApi.md) -----
    /// POST /v1/media/image — multipart 이미지 업로드
    void upload_image(const QByteArray& image_data,
                      const QString& mime_type,
                      const QString& intent_hint,
                      JsonCallback callback);

    // ----- Speech API (SpeechApi.md) -----
    /// POST /v1/speech/utterance — 폰 STT 결과 텍스트 송신
    void send_utterance(const QString& text,
                        double stt_confidence,
                        const QString& context,
                        const QString& image_request_id,
                        JsonCallback callback);

    // ----- Pill API (PillApi.md) -----
    /// POST /v1/pill/identify
    void identify_pill(const QString& image_request_id,
                       const QString& utterance_request_id,
                       bool include_dur_check,
                       JsonCallback callback);

    /// GET /v1/pill/pool
    void get_pill_pool(bool include_inactive, JsonCallback callback);

    /// POST /v1/pill/pool
    void add_pill_to_pool(const QString& item_code,
                          const QString& reg_method,
                          JsonCallback callback);

    /// DELETE /v1/pill/pool/{pool_id}
    void remove_pill_from_pool(int pool_id, JsonCallback callback);

    /// DELETE /v1/pill/pool/all  (X-Confirm-Reset 헤더 자동 첨부)
    void reset_pill_pool(JsonCallback callback);

    // ----- History API (HistoryApi.md) -----
    /// POST /v1/history/record
    void record_history(const QString& item_code,
                        int quantity,
                        const QString& memo,
                        JsonCallback callback);

    /// GET /v1/history/list
    void list_history(const QString& from_date,
                      const QString& to_date,
                      int page,
                      int page_size,
                      JsonCallback callback);

    // ----- Report API (ReportApi.md) -----
    /// GET /v1/report/generate
    void generate_report(const QString& from_date,
                         const QString& to_date,
                         const QString& format,   // "pdf" / "html" / "json"
                         JsonCallback callback);

    // ----- Monitoring API (MonitoringApi.md) -----
    /// GET /health
    void check_health(JsonCallback callback);
    /// GET /metrics
    void check_metrics(JsonCallback callback);

signals:
    /// 401 응답 감지 시 발신 — UI에서 재로그인 유도
    void token_expired();

    /// 네트워크 자체 실패 (서버 응답 없음) — UI에서 연결 상태 표시
    void network_error(const QString& reason);

private:
    /// 공통 헤더(JWT, Content-Type, X-Request-ID) 첨부된 요청 객체 생성
    QNetworkRequest build_request(const QString& path,
                                  const QString& content_type) const;

    /// 메소드별 공통 송신 + 콜백 디스패칭
    void send_request(const QString& method,
                      const QString& path,
                      const QByteArray& body,
                      const QString& content_type,
                      JsonCallback callback,
                      const QList<QPair<QByteArray, QByteArray>>& extra_headers = {});

    QString base_url_;
    QString access_token_;
    QNetworkAccessManager* network_manager_;
};

} // namespace medibridge::network
