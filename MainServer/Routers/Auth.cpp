// =====================================================
// Auth Router — 실 Auth 서비스 사용 (PasswordHasher + JwtIssuer + UserManager)
// =====================================================
// TestMode 시드 사용자(`$pbkdf2$test_hash_placeholder$<plain>` 형식) 도 그대로 동작.
// =====================================================
#include "Auth.h"
#include "../Schemas/AuthSchema.h"
#include "../Services/Auth/Authenticator.h"
#include "../Services/Auth/UserManager.h"
#include "../Services/Auth/JwtIssuer.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

namespace auth_svc = medibridge::services::auth;

static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message,
                                              const std::string& field = "")
{
    Json::Value body, err;
    err["code"]    = err_code;
    err["message"] = message;
    if (!field.empty()) {
        Json::Value details; details["field"] = field;
        err["details"] = details;
    }
    body["error"] = err;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

void Auth::handle_signup(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::SignupRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드/형식 검증 실패", err_field));
        return;
    }

    std::string svc_err;
    auto created = auth_svc::UserManager::create_user(reqobj, svc_err);
    if (!created) {
        const auto code = (svc_err == "EMAIL_TAKEN")    ? drogon::k409Conflict
                        : (svc_err == "DB_UNAVAILABLE") ? drogon::k503ServiceUnavailable
                        : drogon::k500InternalServerError;
        callback(error_response(code, svc_err, "회원가입 실패",
                                svc_err == "EMAIL_TAKEN" ? "email" : ""));
        return;
    }
    auto http = drogon::HttpResponse::newHttpJsonResponse(created->to_json());
    http->setStatusCode(drogon::k201Created);
    callback(http);
}

void Auth::handle_login(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::LoginRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드 누락", err_field));
        return;
    }

    std::string svc_err;
    auto result = auth_svc::Authenticator::authenticate(reqobj, svc_err);
    if (!result) {
        callback(error_response(drogon::k401Unauthorized, svc_err,
                                "이메일 또는 비밀번호가 올바르지 않습니다."));
        return;
    }
    callback(drogon::HttpResponse::newHttpJsonResponse(result->to_json()));
}

void Auth::handle_logout(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // Bearer 토큰을 블랙리스트 등록 (없거나 잘못돼도 204 — 클라 영구 멱등)
    const auto h = req->getHeader("Authorization");
    static const std::string kPrefix = "Bearer ";
    if (h.size() > kPrefix.size() && h.compare(0, kPrefix.size(), kPrefix) == 0) {
        auth_svc::JwtIssuer::invalidate(h.substr(kPrefix.size()));
    }
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

} // namespace medibridge::routers
