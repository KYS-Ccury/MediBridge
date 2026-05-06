#include "OnboardingInferenceClient.h"

namespace medibridge::services::inference {

OnboardingInferenceClient::OnboardingInferenceClient(std::shared_ptr<InferenceClientCommon> common)
    : common_(std::move(common))
{
}

void OnboardingInferenceClient::normalize_drug_name(const std::string& raw_text, Callback callback)
{
    Json::Value body;
    body["raw_text"] = raw_text;
    common_->post_json("/onboarding/normalize", body, std::move(callback));
}

void OnboardingInferenceClient::generate_disambiguation(const Json::Value& candidates,
                                                        Callback callback)
{
    Json::Value body;
    body["candidates"] = candidates;
    common_->post_json("/onboarding/disambiguate", body, std::move(callback));
}

} // namespace medibridge::services::inference
