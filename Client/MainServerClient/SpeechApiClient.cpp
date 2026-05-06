#include "SpeechApiClient.h"
#include "ApiClientCommon.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace medibridge::network {

SpeechApiClient::SpeechApiClient(std::shared_ptr<ApiClientCommon> common, QObject* parent)
    : QObject(parent)
    , common_(std::move(common))
{
}

void SpeechApiClient::send_utterance(const QString& text,
                                     double stt_confidence,
                                     const QString& context,
                                     const QString& image_request_id,
                                     JsonCallback callback)
{
    QJsonObject body{
        {"text", text},
        {"stt_confidence", stt_confidence},
        {"stt_engine", "galaxy_ai"},
        {"context", context.isEmpty() ? "daily_use" : context},
        {"language", "ko-KR"}
    };
    if (!image_request_id.isEmpty()) {
        body["image_request_id"] = image_request_id;
    }
    common_->send_request("POST", "/v1/speech/utterance",
                          QJsonDocument(body).toJson(QJsonDocument::Compact),
                          "application/json", std::move(callback));
}

} // namespace medibridge::network
