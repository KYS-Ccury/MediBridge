#include "InferenceClient.h"

namespace medibridge::services::inference {

InferenceClient& InferenceClient::instance()
{
    static InferenceClient instance;
    return instance;
}

InferenceClient::InferenceClient()
    : common_(std::make_shared<InferenceClientCommon>())
    , vision_(common_)
    , intent_(common_)
    , onboarding_(common_)
    , summary_(common_)
{
}

} // namespace medibridge::services::inference
