#include "UtteranceForwarder.h"
#include "ApiClient.h"

#include <QLoggingCategory>

namespace medibridge::services {

UtteranceForwarder::UtteranceForwarder(network::ApiClient* api_client, QObject* parent)
    : QObject(parent)
    , api_client_(api_client)
{
}

// =====================================================
// forward — 1차 검증 후 메인서버 음성 API 호출
// =====================================================
void UtteranceForwarder::forward(const QString& text,
                                 double stt_confidence,
                                 const QString& context,
                                 const QString& image_request_id,
                                 ForwardCallback callback)
{
    // 1차 검증
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        qWarning() << "[UtteranceForwarder] 빈 텍스트";
        if (callback) callback(QByteArray("{\"error\":\"EMPTY_TEXT\"}"), 400);
        return;
    }
    if (trimmed.size() > 1000) {  // 1000자 초과는 1차 거부 (메인 측에서도 재검증)
        qWarning() << "[UtteranceForwarder] 텍스트 길이 초과:" << trimmed.size();
        if (callback) callback(QByteArray("{\"error\":\"TEXT_TOO_LONG\"}"), 400);
        return;
    }
    if (!api_client_) {
        qWarning() << "[UtteranceForwarder] api_client_ null";
        if (callback) callback(QByteArray("{\"error\":\"NO_API_CLIENT\"}"), 500);
        return;
    }

    qInfo().nospace() << "[UtteranceForwarder] forward — len=" << trimmed.size()
                      << " conf=" << stt_confidence << " ctx=" << context;

    // UI 시그널 — VoiceController 가 받아서 실시간 텍스트 표시
    emit utterance_received(trimmed, true);

    // 메인서버 호출
    api_client_->speech().send_utterance(trimmed, stt_confidence, context, image_request_id,
        [callback](const QByteArray& resp, int status) {
            qInfo().nospace() << "[UtteranceForwarder] 응답 status=" << status
                              << " body=" << resp.size() << "B";
            if (callback) callback(resp, status);
        });
}

} // namespace medibridge::services
