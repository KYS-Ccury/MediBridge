// =====================================================
// MediaApiClient — 구현 (MediaApi v0.2)
// =====================================================
// ⭐ 정상 사진 흐름 (3단계):
//   ① POST /v1/media/intent       — 메인서버에 의향 신호 + 토큰 발급
//   ② PUT  <storage_url>           — 보관 PC(10.10.10.122:8004) 직접 업로드
//   ③ POST /v1/media/commit       — PUT 완료 통지 → status PENDING → READY
//
// 레거시:
//   POST /v1/media/image          — 메인서버 로컬 저장 (TestMode fallback)
//
// 사진 본체가 메인서버를 통과하지 않음 — 메인은 토큰만 발급.
// =====================================================
#include "MediaApiClient.h"
#include "ApiClientCommon.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUuid>

namespace medibridge::network {

MediaApiClient::MediaApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

// =====================================================
// ① POST /v1/media/intent
// =====================================================
void MediaApiClient::request_intent(const QString& mime_type,
                                    qint64 size_bytes,
                                    const QString& purpose,
                                    JsonCallback callback)
{
    QJsonObject body;
    body["mime_type"]  = mime_type;
    body["size_bytes"] = static_cast<double>(size_bytes);   // QJson 은 int64 직접 지원 X
    body["purpose"]    = purpose;

    const QByteArray body_bytes = QJsonDocument(body).toJson(QJsonDocument::Compact);

    qInfo().nospace() << "[MediaApiClient] ① POST /v1/media/intent "
                      << "(mime=" << mime_type << " size=" << size_bytes
                      << " purpose=" << purpose << ")";

    common_->send_request("POST", "/v1/media/intent", body_bytes,
                          "application/json", std::move(callback));
}

// =====================================================
// ② PUT <storage_url> — 보관 PC 직접
// =====================================================
// ApiClientCommon::send_request 는 메인서버 base_url 에만 동작.
// 보관 PC URL 은 다른 호스트라 QNetworkAccessManager 직접 호출.
// Authorization 헤더는 메인서버 JWT 가 아닌 put_token (HMAC) 사용.
// =====================================================
void MediaApiClient::put_to_storage(const QString& storage_url,
                                    const QString& put_token,
                                    const QByteArray& image_data,
                                    const QString& mime_type,
                                    JsonCallback callback)
{
    QNetworkRequest request{QUrl(storage_url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, mime_type);
    request.setRawHeader("Authorization",
                         ("Bearer " + put_token).toUtf8());
    request.setRawHeader("X-Request-ID",
                         QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8());

    qInfo().nospace() << "[MediaApiClient] ② PUT " << storage_url
                      << " (body " << image_data.size() << "B, mime=" << mime_type << ")";

    QNetworkReply* reply =
        common_->network_manager()->put(request, image_data);

    QObject::connect(reply, &QNetworkReply::finished, this,
        [reply, callback, storage_url]() {
            const int status = reply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QByteArray body = reply->readAll();
            const auto net_err = reply->error();

            qInfo().nospace() << "[MediaApiClient] ② PUT " << storage_url
                              << "  status=" << status
                              << "  body=" << body.size() << "B";

            if (net_err != QNetworkReply::NoError && status == 0) {
                qWarning() << "[MediaApiClient] PUT 네트워크 오류:"
                           << reply->errorString();
                if (callback) callback(QByteArray(), 0);
            } else {
                if (callback) callback(body, status);
            }
            reply->deleteLater();
        });
}

// =====================================================
// ③ POST /v1/media/commit
// =====================================================
void MediaApiClient::commit_upload(const QString& photo_id,
                                   JsonCallback callback)
{
    QJsonObject body;
    body["photo_id"] = photo_id;

    const QByteArray body_bytes = QJsonDocument(body).toJson(QJsonDocument::Compact);

    qInfo().nospace() << "[MediaApiClient] ③ POST /v1/media/commit "
                      << "(photo_id=" << photo_id << ")";

    common_->send_request("POST", "/v1/media/commit", body_bytes,
                          "application/json", std::move(callback));
}

// =====================================================
// 레거시 — POST /v1/media/image (TestMode fallback)
// =====================================================
// TODO (선택): 실제 multipart/form-data 구현 — QHttpMultiPart 패턴.
// 신규 코드는 정상 흐름 (intent/commit) 을 사용하므로 본 메소드는 fallback 전용.
// =====================================================
void MediaApiClient::upload_image(const QByteArray& image_data,
                                  const QString& mime_type,
                                  const QString& intent_hint,
                                  JsonCallback callback)
{
    Q_UNUSED(image_data); Q_UNUSED(mime_type); Q_UNUSED(intent_hint);
    qInfo() << "[MediaApiClient] upload_image (레거시) — multipart 미구현. "
               "정상 흐름 (request_intent → put_to_storage → commit_upload) 사용 권장.";
    if (callback) callback(QByteArray("{\"todo\":true}"), 202);
}

} // namespace medibridge::network
