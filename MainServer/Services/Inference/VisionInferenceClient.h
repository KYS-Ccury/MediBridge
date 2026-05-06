// =====================================================
// VisionInferenceClient — POST /vision/detect, /vision/analyze
// =====================================================
#pragma once

#include "InferenceClientCommon.h"
#include <memory>
#include <string>

namespace medibridge::services::inference {

class VisionInferenceClient
{
public:
    using Callback = InferenceClientCommon::Callback;

    explicit VisionInferenceClient(std::shared_ptr<InferenceClientCommon> common);

    /// POST /vision/detect — 이미지 → 검출 결과
    void detect_pills(const std::string& image_path, Callback callback);

    /// POST /vision/analyze — crop → 각인·색·모양 분석
    void analyze_pill_crops(const std::string& crop_id, Callback callback);

private:
    std::shared_ptr<InferenceClientCommon> common_;
};

} // namespace medibridge::services::inference
