#include "SummaryInferenceClient.h"

namespace medibridge::services::inference {

SummaryInferenceClient::SummaryInferenceClient(std::shared_ptr<InferenceClientCommon> common)
    : common_(std::move(common))
{
}

void SummaryInferenceClient::summarize_non_medical(const Json::Value& source, Callback callback)
{
    // TODO (영역 B 분담):
    //   - source 검증 — 의료 안내 키워드("부작용", "금기" 등) 차단
    //   - 출력 검증 — 단정 표현 reject ("복용 가능합니다" 등)
    Json::Value body;
    body["source"] = source;
    common_->post_json("/summary/non-medical", body, std::move(callback));
}

} // namespace medibridge::services::inference
