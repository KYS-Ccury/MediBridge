#include "VisionInferenceClient.h"

namespace medibridge::services::inference {

VisionInferenceClient::VisionInferenceClient(std::shared_ptr<InferenceClientCommon> common)
    : common_(std::move(common))
{
}

void VisionInferenceClient::detect_pills(const std::string& image_path, Callback callback)
{
    common_->post_file("/vision/detect", image_path, "image", std::move(callback));
}

void VisionInferenceClient::analyze_pill_crops(const std::string& crop_id, Callback callback)
{
    Json::Value body;
    body["crop_id"] = crop_id;
    common_->post_json("/vision/analyze", body, std::move(callback));
}

} // namespace medibridge::services::inference
