// =====================================================
// Photo — PUT / GET 구현
// =====================================================
#include "Photo.h"
#include "../Services/TokenVerifier.h"
#include "../Services/StorageManager.h"
#include "../Config.h"

#include <drogon/HttpResponse.h>
#include <iostream>

namespace datastorage::routers {

namespace svc = datastorage::services;

// =====================================================
// 헬퍼
// =====================================================
static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message)
{
    Json::Value body, err;
    err["code"]    = err_code;
    err["message"] = message;
    body["error"]  = err;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

/// filename(예: "ph_abc.jpg") 을 photo_id + ext 로 분리
static bool split_filename(const std::string& filename,
                           std::string& photo_id_out,
                           std::string& ext_out)
{
    const auto dot = filename.rfind('.');
    if (dot == std::string::npos || dot == 0 || dot == filename.size() - 1) return false;
    photo_id_out = filename.substr(0, dot);
    ext_out      = filename.substr(dot + 1);
    // 소문자 정규화 (jpg/JPG/Jpeg 등 차이 흡수)
    for (auto& c : ext_out) c = static_cast<char>(std::tolower(c));
    if (ext_out == "jpeg") ext_out = "jpg";   // 내부적으로는 jpg 로 통일
    return true;
}

/// 토큰의 mime → 허용 확장자 매핑
static bool mime_matches_ext(const std::string& mime, const std::string& ext)
{
    if (mime == "image/jpeg") return ext == "jpg";
    if (mime == "image/png")  return ext == "png";
    return false;
}

// =====================================================
// PUT /storage/photos/{anon}/{filename}
// =====================================================
// 검증 순서:
//   1. Authorization 헤더 → 토큰 추출 → op="put" 검증
//   2. URL 경로의 anon/photo_id 가 토큰 sub/jti 와 일치
//   3. 확장자 ↔ 토큰 mime 일치
//   4. Content-Length / 본문 크기 ≤ 토큰 max  AND  ≤ 서버 max_bytes
//   5. Content-Type ↔ 토큰 mime 일치 (있으면)
//   6. StorageManager::put_atomic
// =====================================================
void Photo::handle_put(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       std::string anon,
                       std::string filename)
{
    // 1) 토큰 검증
    const auto claims = svc::TokenVerifier::verify_header(req->getHeader("Authorization"), "put");
    if (!claims) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN",
                                "토큰이 없거나 만료/위조됨."));
        return;
    }

    // 2) URL 분해
    std::string photo_id, ext;
    if (!split_filename(filename, photo_id, ext)) {
        callback(error_response(drogon::k400BadRequest, "BAD_FILENAME",
                                "파일명 형식 오류 (예: ph_xxx.jpg)"));
        return;
    }

    // 3) 화이트리스트 검증
    if (!svc::StorageManager::is_safe(anon) || !svc::StorageManager::is_safe(photo_id)) {
        callback(error_response(drogon::k400BadRequest, "INVALID_ID",
                                "anon/photo_id 형식 오류"));
        return;
    }
    if (!svc::StorageManager::is_allowed_ext(ext)) {
        callback(error_response(drogon::k400BadRequest, "INVALID_EXT",
                                "허용 확장자: jpg, png"));
        return;
    }

    // 4) 토큰 ↔ URL 일치
    if (claims->anonymous_id != anon) {
        callback(error_response(drogon::k403Forbidden, "ANON_MISMATCH",
                                "URL anon 이 토큰 sub 와 다름"));
        return;
    }
    if (claims->photo_id != photo_id) {
        callback(error_response(drogon::k403Forbidden, "PHOTO_ID_MISMATCH",
                                "URL photo_id 가 토큰 jti 와 다름"));
        return;
    }

    // 5) MIME ↔ 확장자
    if (!mime_matches_ext(claims->mime_type, ext)) {
        callback(error_response(drogon::k400BadRequest, "MIME_EXT_MISMATCH",
                                "토큰 mime 과 URL 확장자 불일치"));
        return;
    }

    // 6) Content-Type 헤더 검증 (있으면)
    const auto ct = req->getHeader("Content-Type");
    if (!ct.empty() && ct.find(claims->mime_type) == std::string::npos) {
        callback(error_response(drogon::k400BadRequest, "CONTENT_TYPE_MISMATCH",
                                "Content-Type 이 토큰 mime 과 다름"));
        return;
    }

    // 7) 크기 검증 — 토큰 max  AND  서버 max
    const auto& body = req->body();
    const long body_size  = static_cast<long>(body.size());
    const long allowed = std::min(claims->max_bytes, Config::instance().max_bytes());
    if (body_size <= 0) {
        callback(error_response(drogon::k400BadRequest, "EMPTY_BODY", "본문이 비어 있음"));
        return;
    }
    if (body_size > allowed) {
        callback(error_response(drogon::k413RequestEntityTooLarge, "SIZE_EXCEEDED",
                                "본문 크기 한도 초과"));
        return;
    }

    // 8) 디스크 저장 (atomic)
    std::string err_msg;
    if (!svc::StorageManager::put_atomic(anon, photo_id, ext,
                                         std::string(body.data(), body.size()),
                                         err_msg)) {
        std::cerr << "[Photo] put_atomic 실패: " << err_msg << std::endl;
        callback(error_response(drogon::k500InternalServerError, "STORAGE_FAILED", err_msg));
        return;
    }

    // 9) 응답
    Json::Value body_resp;
    body_resp["photo_id"] = photo_id;
    body_resp["bytes"]    = static_cast<Json::Int64>(body_size);
    body_resp["status"]   = "stored";
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body_resp);
    resp->setStatusCode(drogon::k201Created);
    callback(resp);
}

// =====================================================
// GET /storage/photos/{anon}/{filename}
// =====================================================
// op="get" 토큰 필요. URL 경로 ↔ 토큰 sub/jti 일치.
// 응답: 바이너리 + Content-Type 매핑.
// =====================================================
void Photo::handle_get(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                       std::string anon,
                       std::string filename)
{
    const auto claims = svc::TokenVerifier::verify_header(req->getHeader("Authorization"), "get");
    if (!claims) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN",
                                "토큰이 없거나 만료/위조됨."));
        return;
    }

    std::string photo_id, ext;
    if (!split_filename(filename, photo_id, ext)) {
        callback(error_response(drogon::k400BadRequest, "BAD_FILENAME", "파일명 형식 오류"));
        return;
    }
    if (!svc::StorageManager::is_safe(anon) || !svc::StorageManager::is_safe(photo_id)
        || !svc::StorageManager::is_allowed_ext(ext)) {
        callback(error_response(drogon::k400BadRequest, "INVALID_PATH", "경로 형식 오류"));
        return;
    }

    if (claims->anonymous_id != anon || claims->photo_id != photo_id) {
        callback(error_response(drogon::k403Forbidden, "TOKEN_PATH_MISMATCH",
                                "토큰이 이 경로에 발급되지 않았음"));
        return;
    }
    if (!mime_matches_ext(claims->mime_type, ext)) {
        callback(error_response(drogon::k400BadRequest, "MIME_EXT_MISMATCH",
                                "토큰 mime 과 URL 확장자 불일치"));
        return;
    }

    auto data = svc::StorageManager::get(anon, photo_id, ext);
    if (!data) {
        callback(error_response(drogon::k404NotFound, "NOT_FOUND", "파일 없음"));
        return;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k200OK);
    resp->setContentTypeString(claims->mime_type);
    resp->setBody(std::move(*data));
    callback(resp);
}

} // namespace datastorage::routers
