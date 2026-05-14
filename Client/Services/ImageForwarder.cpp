#include "ImageForwarder.h"
#include "ApiClient.h"
#include "WorkerPool.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>

namespace medibridge::services {

ImageForwarder::ImageForwarder(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

// =====================================================
// forward — 사진 흐름 ⑤+⑥ 정상 경로 (intent → PUT → commit)
// =====================================================
// 1. 크기·MIME 검증
// 2. /v1/media/intent → photo_id + storage_url + put_token 수신
// 3. PUT <storage_url> → 보관 PC 직접 업로드 (메인 우회)
// 4. /v1/media/commit → status PENDING → READY
//
// 콜백은 마지막 commit 응답을 전달 — 호출자는 그 응답의 photo_id /
// request_id 로 /v1/pill/identify 트리거 가능.
// =====================================================
void ImageForwarder::forward(const QByteArray& image_data,
                             const QString& mime_type,
                             const QString& intent_hint,
                             ForwardCallback callback)
{
    constexpr qint64 MAX_BYTES = 10 * 1024 * 1024;   // 10MB
    if (image_data.isEmpty()) {
        qWarning() << "[ImageForwarder] 빈 이미지";
        if (callback) callback(QByteArray("{\"error\":\"EMPTY_BODY\"}"), 400);
        return;
    }
    if (image_data.size() > MAX_BYTES) {
        qWarning() << "[ImageForwarder] 크기 초과:" << image_data.size();
        if (callback) callback(QByteArray("{\"error\":\"SIZE_EXCEEDED\"}"), 413);
        return;
    }
    if (mime_type != "image/jpeg" && mime_type != "image/png") {
        qWarning() << "[ImageForwarder] mime 미지원:" << mime_type;
        if (callback) callback(QByteArray("{\"error\":\"INVALID_MIME_TYPE\"}"), 400);
        return;
    }
    if (!api_client_) {
        qWarning() << "[ImageForwarder] api_client_ null";
        if (callback) callback(QByteArray("{\"error\":\"NO_API_CLIENT\"}"), 500);
        return;
    }

    Q_UNUSED(intent_hint);  // 새 흐름은 purpose='IDENTIFY' 고정 (필요 시 매핑)

    qInfo().nospace() << "[ImageForwarder] forward 시작 — "
                      << image_data.size() << "B " << mime_type;

    // ① intent
    api_client_->media().request_intent(mime_type, image_data.size(), QStringLiteral("IDENTIFY"),
        [this, image_data, mime_type, callback]
        (const QByteArray& intent_resp, int intent_status) {
            if (intent_status != 201) {
                qWarning() << "[ImageForwarder] intent 실패 status=" << intent_status;
                if (callback) callback(intent_resp, intent_status);
                return;
            }
            const auto j = QJsonDocument::fromJson(intent_resp).object();
            const QString photo_id    = j.value("photo_id").toString();
            const QString storage_url = j.value("storage_url").toString();
            const QString put_token   = j.value("put_token").toString();
            const QString req_id      = j.value("request_id").toString();
            if (photo_id.isEmpty() || storage_url.isEmpty() || put_token.isEmpty()) {
                if (callback) callback(QByteArray("{\"error\":\"INTENT_INVALID_RESPONSE\"}"), 502);
                return;
            }

            // ② PUT 보관 PC
            api_client_->media().put_to_storage(storage_url, put_token, image_data, mime_type,
                [this, photo_id, req_id, callback]
                (const QByteArray& put_resp, int put_status) {
                    if (put_status != 201) {
                        qWarning() << "[ImageForwarder] PUT 실패 status=" << put_status;
                        if (callback) callback(put_resp, put_status);
                        return;
                    }
                    // ③ commit
                    api_client_->media().commit_upload(photo_id,
                        [photo_id, req_id, callback]
                        (const QByteArray& commit_resp, int commit_status) {
                            qInfo() << "[ImageForwarder] forward 완료 photo_id=" << photo_id
                                    << " status=" << commit_status;
                            // 최종 응답에 request_id 도 포함 (호출자가 identify 트리거 시 필요)
                            QJsonObject merged = QJsonDocument::fromJson(commit_resp).object();
                            merged["request_id"] = req_id;
                            merged["photo_id"]   = photo_id;
                            const QByteArray body = QJsonDocument(merged).toJson(QJsonDocument::Compact);
                            if (callback) callback(body, commit_status);
                        });
                });
        });
}

} // namespace medibridge::services
