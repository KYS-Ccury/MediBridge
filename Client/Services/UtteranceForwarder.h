// =====================================================
// UtteranceForwarder — 폰 STT 결과 텍스트를 MainServer로 중계
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <functional>

namespace medibridge::network { class ApiClient; }

namespace medibridge::services {

class UtteranceForwarder : public QObject
{
    Q_OBJECT
public:
    using ForwardCallback = std::function<void(const QByteArray& main_server_response,
                                               int status_code)>;

    explicit UtteranceForwarder(network::ApiClient* api_client,
                                QObject* parent = nullptr);

    /**
     * 폰 STT 결과 텍스트를 MainServer로 전달.
     *   1. text 길이·내용 검증 (인젝션 패턴 1차 필터)
     *   2. ApiClient::speech().send_utterance(...)
     */
    void forward(const QString& text,
                 double stt_confidence,
                 const QString& context,
                 const QString& image_request_id,
                 ForwardCallback callback);

private:
    network::ApiClient* api_client_;
};

} // namespace medibridge::services
