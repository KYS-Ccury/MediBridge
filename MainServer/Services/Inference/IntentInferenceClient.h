// =====================================================
// IntentInferenceClient — POST /intent/classify (Stage 0.5)
// =====================================================
#pragma once

#include "InferenceClientCommon.h"
#include <memory>
#include <string>

namespace medibridge::services::inference {

class IntentInferenceClient
{
public:
    using Callback = InferenceClientCommon::Callback;

    explicit IntentInferenceClient(std::shared_ptr<InferenceClientCommon> common);

    /// POST /intent/classify — 분류만 수행 (응답 생성 X)
    /// ⚠ JSON 스키마 위반 시 reject (FR-A5-02)
    void classify_intent(const std::string& utterance_text,
                         const std::string& image_request_id,
                         Callback callback);

private:
    std::shared_ptr<InferenceClientCommon> common_;
};

} // namespace medibridge::services::inference
