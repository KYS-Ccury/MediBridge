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

    /// POST /vision/detect — 이미지 → 검출 결과 (레거시 — 로컬 파일 경로)
    void detect_pills(const std::string& image_path, Callback callback);

    /// POST /vision/analyze — crop → 각인·색·모양 분석
    void analyze_pill_crops(const std::string& crop_id, Callback callback);

    // =====================================================
    // ⭐ 데이터 보관 PC 연동 — Vision PC 가 보관 PC 에서 직접 GET
    // =====================================================
    /**
     * @brief 보관 PC 에 있는 사진을 Vision PC 가 추론하도록 의뢰.
     *
     * Vision PC 는 storage_url + get_token 으로 보관 PC GET → 추론 수행 → 결과 반환.
     * 메인서버는 사진 본체를 전송하지 않는다 (대역폭 분산).
     *
     * 페이로드 (POST /vision/detect_remote):
     *   {
     *     "photo_id":   "ph_...",
     *     "storage_url":"http://10.10.10.122:8004/storage/photos/anon_xxx/ph_xxx.jpg",
     *     "get_token":  "<HS256 JWT — op=get, exp=now+300>",
     *     "mime":       "image/jpeg",
     *     "purpose":    "IDENTIFY"
     *   }
     */
    struct RemoteDetectParams {
        std::string photo_id;
        std::string storage_url;
        std::string get_token;
        std::string mime;
        std::string purpose;        // "IDENTIFY" | "TRAIN" | "OTHER"
    };
    void detect_pills_remote(const RemoteDetectParams& p, Callback callback);

private:
    std::shared_ptr<InferenceClientCommon> common_;
};

} // namespace medibridge::services::inference
