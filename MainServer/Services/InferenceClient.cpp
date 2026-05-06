#include "InferenceClient.h"
#include "../Config.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <iostream>

namespace medibridge::services {

InferenceClient& InferenceClient::instance()
{
    static InferenceClient instance;
    return instance;
}

InferenceClient::InferenceClient()
{
    const auto& config = Config::instance();
    http_client_ = drogon::HttpClient::newHttpClient(config.inference_server_url());
}

void InferenceClient::detect_pills(const std::string& image_path, Callback callback)
{
    // TODO (영역 B 분담):
    //   1. drogon::HttpRequest::newFileUploadRequest({{image_path, "image"}}) 사용
    //   2. setPath("/vision/detect"), setMethod(drogon::Post)
    //   3. http_client_->sendRequest(req, lambda) 비동기
    //   4. 응답 파싱 → callback 호출
    std::cout << "[InferenceClient] detect_pills TODO: " << image_path << std::endl;
    if (callback) callback(Json::Value(), 501);
}

void InferenceClient::analyze_pill_crops(const std::string& crop_image_path, Callback callback)
{
    // TODO: POST /vision/analyze
    if (callback) callback(Json::Value(), 501);
}

void InferenceClient::classify_intent(const std::string& utterance_text,
                                      const std::string& image_request_id,
                                      Callback callback)
{
    // TODO: POST /intent/classify
    //   body: { "text": ..., "image_request_id": ... }
    //   응답: { "category": "PILL_IDENTIFY", "confidence": 0.91 }
    //   ⚠ 응답이 JSON 스키마 위반 시 reject (FR-A5-02)
    if (callback) callback(Json::Value(), 501);
}

void InferenceClient::normalize_drug_name(const std::string& raw_text, Callback callback)
{
    // TODO: POST /onboarding/normalize
    if (callback) callback(Json::Value(), 501);
}

void InferenceClient::generate_disambiguation(const Json::Value& candidates, Callback callback)
{
    // TODO: POST /onboarding/disambiguate
    if (callback) callback(Json::Value(), 501);
}

void InferenceClient::summarize_non_medical(const Json::Value& source, Callback callback)
{
    // TODO: POST /summary/non-medical (RAG 허용)
    if (callback) callback(Json::Value(), 501);
}

} // namespace medibridge::services
