// =====================================================
// InferenceClientCommon — 추론 서버 호출 공통 (HttpClient 공유)
// =====================================================
// 4개 카테고리 클라이언트(Vision/Intent/Onboarding/Summary)가 본 객체를 공유.
// =====================================================
#pragma once

#include <drogon/HttpClient.h>
#include <json/json.h>
#include <string>
#include <functional>
#include <memory>

namespace medibridge::services::inference {

class InferenceClientCommon
{
public:
    using Callback = std::function<void(const Json::Value& response, int status_code)>;

    /// Config::inference_server_url 을 베이스 URL로 사용
    InferenceClientCommon();

    /// 공통 POST (JSON 바디)
    void post_json(const std::string& path,
                   const Json::Value& body,
                   Callback callback);

    /// 공통 POST (파일 업로드 — 이미지 등)
    void post_file(const std::string& path,
                   const std::string& file_path,
                   const std::string& form_field,
                   Callback callback);

    drogon::HttpClientPtr http_client() const { return http_client_; }

private:
    drogon::HttpClientPtr http_client_;
};

} // namespace medibridge::services::inference
