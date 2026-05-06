// =====================================================
// MediaForwarder 구현 — 골격
// =====================================================
#include "MediaForwarder.h"
#include "../MainServerClient/ApiClient.h"
#include "../Threading/WorkerPool.h"

#include <QLoggingCategory>

namespace medibridge::services {

MediaForwarder::MediaForwarder(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

void MediaForwarder::forward_image(const QByteArray& image_data,
                                   const QString& mime_type,
                                   const QString& intent_hint,
                                   ForwardCallback callback)
{
    // TODO (영역 C 분담):
    //   1. 이미지 크기·MIME 검증
    //   2. (선택) WorkerPool::instance().submit(...)으로 리사이즈·재인코딩
    //   3. api_client_->upload_image(...) 호출
    //   4. 응답을 callback으로 전달
    //
    // 가공 예시 (워커 위임):
    //   auto future = threading::WorkerPool::instance().submit(
    //       [image_data]() { return resize_to_max(image_data, 1920, 1080); }
    //   );
    qInfo() << "[MediaForwarder] forward_image — TODO 구현 ("
            << image_data.size() << "bytes,"
            << mime_type << ", intent:" << intent_hint << ")";

    if (api_client_) {
        api_client_->upload_image(image_data, mime_type, intent_hint,
            [callback](const QByteArray& response, int status) {
                if (callback) callback(response, status);
            });
    }
}

void MediaForwarder::forward_utterance(const QString& text,
                                       double stt_confidence,
                                       const QString& context,
                                       const QString& image_request_id,
                                       ForwardCallback callback)
{
    // TODO (영역 C 분담):
    //   1. text 길이·내용 검증 (인젝션 패턴 1차 필터 — 본격 방어는 MainServer가 담당)
    //   2. api_client_->send_utterance(...) 호출
    //   3. 응답을 callback으로 전달
    qInfo() << "[MediaForwarder] forward_utterance — TODO 구현 ("
            << text << ", confidence:" << stt_confidence
            << ", context:" << context << ")";

    if (api_client_) {
        api_client_->send_utterance(text, stt_confidence, context, image_request_id,
            [callback](const QByteArray& response, int status) {
                if (callback) callback(response, status);
            });
    }
}

} // namespace medibridge::services
