// =====================================================
// Auth Router — /v1/auth/* (AuthApi.md)
// =====================================================
// Drogon HttpController 자동 등록 (Main.cpp 수정 불필요).
// 라우터는 얇게 — 검증·JSON 변환만 담당, 비즈니스 로직은 Services::AuthService.
// =====================================================
#pragma once

#include <drogon/HttpController.h>

namespace medibridge::routers {

class Auth : public drogon::HttpController<Auth>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Auth::handle_signup, "/v1/auth/signup", drogon::Post);
    ADD_METHOD_TO(Auth::handle_login,  "/v1/auth/login",  drogon::Post);
    ADD_METHOD_TO(Auth::handle_logout, "/v1/auth/logout", drogon::Post);
    METHOD_LIST_END

    void handle_signup(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_login(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_logout(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
