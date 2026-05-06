// =====================================================
// OnboardingInferenceClient — POST /onboarding/normalize, /disambiguate
// =====================================================
// ✅ RAG 허용 영역 (등록). 의료 안내 영역 진입 X.
// =====================================================
#pragma once

#include "InferenceClientCommon.h"
#include <memory>
#include <string>

namespace medibridge::services::inference {

class OnboardingInferenceClient
{
public:
    using Callback = InferenceClientCommon::Callback;

    explicit OnboardingInferenceClient(std::shared_ptr<InferenceClientCommon> common);

    /// POST /onboarding/normalize — 폰 STT 약명 정규화
    void normalize_drug_name(const std::string& raw_text, Callback callback);

    /// POST /onboarding/disambiguate — 동명·동성분 분기 질문 생성
    void generate_disambiguation(const Json::Value& candidates, Callback callback);

private:
    std::shared_ptr<InferenceClientCommon> common_;
};

} // namespace medibridge::services::inference
