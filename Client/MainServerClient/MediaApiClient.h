// =====================================================
// MediaApiClient — /v1/media/* 호출 (MediaApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class MediaApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit MediaApiClient(std::shared_ptr<ApiClientCommon> common,
                            QObject* parent = nullptr);

    // =====================================================
    // ⭐ 정상 사진 흐름 (MediaApi v0.2) — 3단계
    // =====================================================
    // ① 의향 신호 + 토큰 발급
    //    응답: { photo_id, request_id, storage_url, put_token, expires_at, max_bytes }
    void request_intent(const QString& mime_type,
                        qint64 size_bytes,
                        const QString& purpose,    // "IDENTIFY" | "TRAIN" | "OTHER"
                        JsonCallback callback);

    // ② 보관 PC (10.10.10.122:8004) 에 직접 PUT — 메인서버 통과 X
    //    storage_url 은 ① 응답에 박힌 절대 URL 사용
    //    put_token 은 ① 응답의 토큰 — Authorization: Bearer 헤더로
    void put_to_storage(const QString& storage_url,
                        const QString& put_token,
                        const QByteArray& image_data,
                        const QString& mime_type,
                        JsonCallback callback);

    // ③ PUT 완료 통지 — photo_storage status PENDING → READY (idempotent)
    void commit_upload(const QString& photo_id,
                       JsonCallback callback);

    // =====================================================
    // 레거시 / TestMode fallback — POST /v1/media/image
    // =====================================================
    void upload_image(const QByteArray& image_data,
                      const QString& mime_type,
                      const QString& intent_hint,
                      JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
