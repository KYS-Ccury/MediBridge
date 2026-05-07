// =====================================================
// InferenceClient — 추론 서버 클라이언트 조립자 (싱글톤 Facade)
// =====================================================
// LLM PC (10.10.10.120) 와 Vision PC (10.10.10.128) 를 별도로 호출.
//   vision()                            → Vision PC
//   intent()/onboarding()/summary()     → LLM PC
//
// 사용 예:
//   auto& client = InferenceClient::instance();
//   client.vision().detect_pills(image_path, callback);
//   client.intent().classify_intent(text, "", callback);
// =====================================================
#pragma once

#include <memory>

#include "InferenceClientCommon.h"
#include "VisionInferenceClient.h"
#include "IntentInferenceClient.h"
#include "OnboardingInferenceClient.h"
#include "SummaryInferenceClient.h"

namespace medibridge::services::inference {

class InferenceClient
{
public:
    static InferenceClient& instance();

    VisionInferenceClient&     vision()     { return vision_; }
    IntentInferenceClient&     intent()     { return intent_; }
    OnboardingInferenceClient& onboarding() { return onboarding_; }
    SummaryInferenceClient&    summary()    { return summary_; }

private:
    InferenceClient();
    ~InferenceClient() = default;
    InferenceClient(const InferenceClient&) = delete;
    InferenceClient& operator=(const InferenceClient&) = delete;

    std::shared_ptr<InferenceClientCommon> llm_common_;
    std::shared_ptr<InferenceClientCommon> vision_common_;
    VisionInferenceClient     vision_;
    IntentInferenceClient     intent_;
    OnboardingInferenceClient onboarding_;
    SummaryInferenceClient    summary_;
};

} // namespace medibridge::services::inference
