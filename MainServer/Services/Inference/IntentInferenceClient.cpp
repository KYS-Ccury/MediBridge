#include "IntentInferenceClient.h"

namespace medibridge::services::inference {

IntentInferenceClient::IntentInferenceClient(std::shared_ptr<InferenceClientCommon> common)
    : common_(std::move(common))
{
}

void IntentInferenceClient::classify_intent(const std::string& utterance_text,
                                            const std::string& image_request_id,
                                            Callback callback)
{
    Json::Value body;
    body["text"] = utterance_text;
    if (!image_request_id.empty()) {
        body["image_request_id"] = image_request_id;
    }

    // TODO (영역 B 분담):
    //   - 응답이 JSON 스키마 위반 시 reject (FR-A5-02)
    //   - 응답 검증: category가 정의된 enum 값인지 확인
    common_->post_json("/intent/classify", body, std::move(callback));
}

} // namespace medibridge::services::inference
