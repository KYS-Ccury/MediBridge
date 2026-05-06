// =====================================================
// InferenceClient — 추론 서버(InferenceServer) REST 호출 클라이언트
// =====================================================
// Drogon HttpClient 사용. 비동기 코루틴.
// 본 클라이언트가 Vision/Intent/Onboarding/Summary 엔드포인트 모두 호출.
// =====================================================
#pragma once

#include <drogon/HttpClient.h>
#include <string>
#include <functional>

namespace medibridge::services {

class InferenceClient
{
public:
    using Callback = std::function<void(const Json::Value& response, int status_code)>;

    static InferenceClient& instance();

    /// 알약 검출 (POST /vision/detect) — 이미지 바이너리 → 검출 결과
    void detect_pills(const std::string& image_path, Callback callback);

    /// 각인·색·모양 분석 (POST /vision/analyze)
    void analyze_pill_crops(const std::string& crop_image_path, Callback callback);

    /// 의도 분류 (POST /intent/classify) — 폰 STT 텍스트 + (선택)이미지 컨텍스트
    void classify_intent(const std::string& utterance_text,
                         const std::string& image_request_id,
                         Callback callback);

    /// 약명 정규화 (POST /onboarding/normalize)
    void normalize_drug_name(const std::string& raw_text, Callback callback);

    /// 동명·동성분 분기 질문 생성 (POST /onboarding/disambiguate)
    void generate_disambiguation(const Json::Value& candidates, Callback callback);

    /// 비위험 정보 자연어 요약 (POST /summary/non-medical) — RAG 사용
    void summarize_non_medical(const Json::Value& source, Callback callback);

private:
    InferenceClient();
    drogon::HttpClientPtr http_client_;
};

} // namespace medibridge::services
