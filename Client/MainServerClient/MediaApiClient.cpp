#include "MediaApiClient.h"
#include "ApiClientCommon.h"

#include <QLoggingCategory>

namespace medibridge::network {

MediaApiClient::MediaApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void MediaApiClient::upload_image(const QByteArray& image_data,
                                  const QString& mime_type,
                                  const QString& intent_hint,
                                  JsonCallback callback)
{
    // TODO (영역 C 분담):
    //   1. QHttpMultiPart 생성 (image part + intent_hint part)
    //   2. common_->network_manager()->post(...) 직접 호출 (multipart는 send_request 우회)
    //   3. JWT 헤더 첨부는 build_request 와 동일 패턴
    //   4. 응답 → 콜백
    Q_UNUSED(image_data); Q_UNUSED(mime_type); Q_UNUSED(intent_hint);
    qInfo() << "[MediaApiClient] upload_image — TODO multipart";
    if (callback) callback(QByteArray("{\"todo\":true}"), 202);
}

} // namespace medibridge::network
