// =====================================================
// SpeechApiClient — /v1/speech/* 호출 (SpeechApi.md)
// =====================================================
#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <functional>

namespace medibridge::network {

class ApiClientCommon;

class SpeechApiClient : public QObject
{
    Q_OBJECT
public:
    using JsonCallback = std::function<void(const QByteArray&, int)>;

    explicit SpeechApiClient(std::shared_ptr<ApiClientCommon> common,
                             QObject* parent = nullptr);

    /// POST /v1/speech/utterance — 폰 STT 결과 텍스트 송신
    void send_utterance(const QString& text,
                        double stt_confidence,
                        const QString& context,
                        const QString& image_request_id,
                        JsonCallback callback);

private:
    std::shared_ptr<ApiClientCommon> common_;
};

} // namespace medibridge::network
