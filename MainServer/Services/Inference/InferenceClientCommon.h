// =====================================================
// InferenceClientCommon — 추론 서버 호출 공통 (HttpClient)
// =====================================================
// LLM PC (10.10.10.128) / Vision PC (10.10.10.120) 처럼 카테고리별로
// 별도 베이스 URL 을 사용하므로, 본 객체를 카테고리당 1개 생성한다.
// 4개 카테고리 클라이언트(Vision/Intent/Onboarding/Summary)가 각자
// 적절한 Common 을 공유한다.
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

    /// base_url: "http://10.10.10.128:8002" 등. timeout_ms.
    explicit InferenceClientCommon(const std::string& base_url,
                                   int timeout_ms);

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
    const std::string& base_url() const { return base_url_; }

private:
    std::string           base_url_;
    int                   timeout_ms_;
    drogon::HttpClientPtr http_client_;
};

} // namespace medibridge::services::inference
