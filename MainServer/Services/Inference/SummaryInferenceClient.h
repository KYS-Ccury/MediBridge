// =====================================================
// SummaryInferenceClient — POST /summary/non-medical
// =====================================================
// ⚠️ 비의료 영역 한정. 의료 안내 영역 데이터 입력 금지.
// =====================================================
#pragma once

#include "InferenceClientCommon.h"
#include <memory>

namespace medibridge::services::inference {

class SummaryInferenceClient
{
public:
    using Callback = InferenceClientCommon::Callback;

    explicit SummaryInferenceClient(std::shared_ptr<InferenceClientCommon> common);

    /// POST /summary/non-medical — e약은요 비위험 정보 자연어 요약
    void summarize_non_medical(const Json::Value& source, Callback callback);

private:
    std::shared_ptr<InferenceClientCommon> common_;
};

} // namespace medibridge::services::inference
