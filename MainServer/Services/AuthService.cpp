#include "AuthService.h"
#include "../Database/Connection.h"
#include "../Config.h"

#include <iostream>

namespace medibridge::services {

std::optional<schemas::SignupResponse>
AuthService::create_user(const schemas::SignupRequest& request,
                         std::string& error_code_out)
{
    // TODO (영역 B 분담):
    //   1. SELECT FROM users WHERE email = ? — 중복 체크 (있으면 EMAIL_ALREADY_EXISTS)
    //   2. user_id 생성 (UUID 또는 auto_increment)
    //   3. password_hash = hash_password(request.password)
    //   4. INSERT INTO users (...)
    //   5. SignupResponse 반환
    error_code_out = "NOT_IMPLEMENTED";
    return std::nullopt;
}

std::optional<schemas::LoginResponse>
AuthService::authenticate(const schemas::LoginRequest& request,
                          std::string& error_code_out)
{
    // TODO (영역 B 분담):
    //   1. SELECT password_hash, user_id FROM users WHERE email = ?
    //   2. verify_password(request.password, hashed) — bcrypt 비교
    //   3. 실패 시 INVALID_CREDENTIALS
    //   4. issue_jwt(user_id, Config::jwt_expire_seconds())
    //   5. LoginResponse 구성
    error_code_out = "NOT_IMPLEMENTED";
    return std::nullopt;
}

std::string AuthService::issue_jwt(const std::string& user_id, int expire_seconds)
{
    // TODO: jwt-cpp 라이브러리 사용
    //   auto token = jwt::create()
    //       .set_issuer("medibridge")
    //       .set_subject(user_id)
    //       .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(expire_seconds))
    //       .sign(jwt::algorithm::hs256{Config::instance().jwt_secret()});
    return "TODO_JWT_TOKEN";
}

std::optional<std::string> AuthService::verify_jwt(const std::string& token)
{
    // TODO: jwt-cpp 검증, 블랙리스트 체크, 만료 체크
    return std::nullopt;
}

void AuthService::invalidate_token(const std::string& token)
{
    // TODO: 블랙리스트 등록 (Redis 또는 메모리 캐시)
}

std::optional<std::string>
AuthService::extract_user_id_from_header(const std::string& auth_header)
{
    // TODO: "Bearer <token>" 파싱 → verify_jwt → user_id
    return std::nullopt;
}

std::string AuthService::hash_password(const std::string& plain_password)
{
    // TODO: bcrypt cost=12 권장
    return "TODO_HASH";
}

bool AuthService::verify_password(const std::string& plain_password,
                                  const std::string& hashed)
{
    // TODO: bcrypt::validatePassword
    return false;
}

} // namespace medibridge::services
