#include "ImageForwarder.h"
#include "ApiClient.h"
#include "WorkerPool.h"

#include <QLoggingCategory>

namespace medibridge::services {

ImageForwarder::ImageForwarder(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

void ImageForwarder::forward(const QByteArray& image_data,
                             const QString& mime_type,
                             const QString& intent_hint,
                             ForwardCallback callback)
{
    // TODO (영역 C 분담):
    //   1. 이미지 크기·MIME 검증 (10MB 초과 거부)
    //   2. (선택) WorkerPool::instance().submit(...) — 리사이즈·재인코딩
    //   3. api_client_->media().upload_image(...)
    qInfo() << "[ImageForwarder] forward TODO ("
            << image_data.size() << "bytes," << mime_type
            << ", intent:" << intent_hint << ")";

    if (api_client_) {
        api_client_->media().upload_image(image_data, mime_type, intent_hint,
            [callback](const QByteArray& resp, int status) {
                if (callback) callback(resp, status);
            });
    }
}

} // namespace medibridge::services
