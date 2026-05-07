#include "InferenceClient.h"
#include "../../Config.h"

namespace medibridge::services::inference {

InferenceClient& InferenceClient::instance()
{
    static InferenceClient instance;
    return instance;
}

InferenceClient::InferenceClient()
    : llm_common_(std::make_shared<InferenceClientCommon>(
            Config::instance().inference_llm_base(),
            Config::instance().inference_request_timeout_ms()))
    , vision_common_(std::make_shared<InferenceClientCommon>(
            Config::instance().inference_vision_base(),
            Config::instance().inference_request_timeout_ms()))
    , vision_(vision_common_)
    , intent_(llm_common_)
    , onboarding_(llm_common_)
    , summary_(llm_common_)
{
}

} // namespace medibridge::services::inference
