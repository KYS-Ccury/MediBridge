#include "Auth.h"
#include "../Schemas/AuthSchema.h"
#include "../Services/AuthService.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

void Auth::handle_signup(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. req->getJsonObject() → SignupRequest::from_json()
    //   2. is_valid() 검증
    //   3. AuthService::create_user(req) 호출
    //   4. SignupResponse → JSON → HttpResponse 201
    //   5. 이메일 중복 시 409 EMAIL_ALREADY_EXISTS
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    resp->setBody(R"({"error":{"code":"NOT_IMPLEMENTED","message":"signup TODO"}})");
    callback(resp);
}

void Auth::handle_login(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. LoginRequest 파싱·검증
    //   2. AuthService::authenticate() — bcrypt 비교
    //   3. AuthService::issue_jwt() — JWT 발급
    //   4. LoginResponse 200
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    resp->setBody(R"({"error":{"code":"NOT_IMPLEMENTED","message":"login TODO"}})");
    callback(resp);
}

void Auth::handle_logout(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증 (헤더 Authorization: Bearer <token>)
    //   2. AuthService::invalidate_token() — 블랙리스트 등록
    //   3. 204 No Content
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
