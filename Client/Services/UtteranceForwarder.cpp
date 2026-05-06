#include "UtteranceForwarder.h"
#include "ApiClient.h"

#include <QLoggingCategory>

namespace medibridge::services {

UtteranceForwarder::UtteranceForwarder(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

void UtteranceForwarder::forward(const QString& text,
                                 double stt_confidence,
                                 const QString& context,
                                 const QString& image_request_id,
                                 ForwardCallback callback)
{
    // TODO (영역 C 분담):
    //   1. text 길이·내용 검증 (인젝션 패턴 1차 필터 — 본격 방어는 MainServer)
    //   2. api_client_->speech().send_utterance(...)
    qInfo() << "[UtteranceForwarder] forward TODO ("
            << text << ", conf:" << stt_confidence << ", ctx:" << context << ")";
    emit utterance_received(text, true);
    
    if (api_client_) {
        api_client_->speech().send_utterance(text, stt_confidence, context, image_request_id,
            [callback](const QByteArray& resp, int status) {
                if (callback) callback(resp, status);
            });
    }
}

} // namespace medibridge::services
