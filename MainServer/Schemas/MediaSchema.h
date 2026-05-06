// =====================================================
// MediaSchema — Media API 요청/응답 (MediaApi.md)
// =====================================================
#pragma once

#include <json/json.h>
#include <string>

namespace medibridge::schemas {

// POST /v1/media/image — multipart 또는 JSON+base64
struct ImageUploadRequest {
    // multipart 의 경우 Drogon HttpRequestPtr->getUploadedFile() 사용
    // 본 struct는 메타 필드만 표현
    std::string mime_type;            // image/jpeg | image/png
    std::string intent_hint;          // identify / register
    std::string request_id;           // 클라가 부여 또는 서버가 생성

    static ImageUploadRequest from_request(const Json::Value& meta);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct ImageUploadResponse {
    std::string request_id;
    std::string status;               // "accepted" / "completed"
    std::string next_poll_url;        // 비동기 처리 시 폴링 URL

    Json::Value to_json() const;
};

} // namespace medibridge::schemas
