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

// =====================================================
// POST /v1/media/intent — 사진 업로드 의향 신호 (⑤+⑥)
// =====================================================
// 클라가 사진 본체를 보내기 전에 메인에 "올려도 됨?" 묻고
// 메인이 보관 PC URL + 단기 PUT 토큰을 발급.
// =====================================================
struct MediaIntentRequest {
    std::string mime_type;        // image/jpeg | image/png
    long        size_bytes = 0;   // 예상 본문 크기 (서버 max 와 비교)
    std::string purpose;          // "IDENTIFY" | "TRAIN" | "OTHER" (default IDENTIFY)
    std::string request_id;       // 클라가 부여 또는 서버 생성

    static MediaIntentRequest from_json(const Json::Value& j);
    bool is_valid(std::string& error_field, std::string& error_code, long max_bytes) const;
};

struct MediaIntentResponse {
    std::string photo_id;         // 서버 생성 — 보관 PC 파일명·DB PK
    std::string request_id;       // 입력 그대로 또는 서버 생성본
    std::string storage_url;      // 클라가 PUT 할 절대 URL
    std::string put_token;        // Authorization: Bearer <put_token>
    std::string expires_at;       // ISO 8601 — 클라 UI 안내용
    long        max_bytes = 0;    // 클라가 보낼 수 있는 최대 바이트

    Json::Value to_json() const;
};

// =====================================================
// POST /v1/media/commit — PUT 완료 통지 (선택)
// =====================================================
// 클라가 보관 PC PUT 성공 후 호출 → status=PENDING → READY 전이.
// 미호출 시: 청소 잡이 expires_at 경과 후 EXPIRED 처리.
// =====================================================
struct MediaCommitRequest {
    std::string photo_id;
    std::string etag;             // (선택) 보관 PC 가 응답으로 준 ETag — 무결성 보조 확인

    static MediaCommitRequest from_json(const Json::Value& j);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct MediaCommitResponse {
    std::string photo_id;
    std::string status;           // "READY"
    Json::Value to_json() const;
};

// =====================================================
// POST /v1/media/get_token — Vision PC 용 GET 토큰 발급
// =====================================================
// 메인이 /v1/pill/identify 시점에 자동 발급하는 게 정상 경로지만,
// 디버그/리커버리/Vision PC 단독 호출을 위해 별도 노출.
//   - 본인 소유 photo 만
//   - status='READY' 여야 함 (PENDING/EXPIRED 거부)
//   - GET 토큰만 발급 (PUT 토큰은 발급 안 함)
// =====================================================
struct MediaGetTokenRequest {
    std::string photo_id;
    static MediaGetTokenRequest from_json(const Json::Value& j);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct MediaGetTokenResponse {
    std::string photo_id;
    std::string storage_url;
    std::string get_token;
    std::string mime_type;
    std::string expires_at;
    Json::Value to_json() const;
};

} // namespace medibridge::schemas
