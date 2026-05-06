// =====================================================
// AuthService — 인증 비즈니스 로직 (모듈 6)
// =====================================================
// 비밀번호 해시(bcrypt), JWT 발급·검증, 사용자 CRUD.
// =====================================================
#pragma once

#include <string>
#include <optional>
#include "../Schemas/AuthSchema.h"

namespace medibridge::services {

class AuthService
{
public:
    /// 회원가입 — 이메일 중복 체크, bcrypt 해시 저장
    static std::optional<schemas::SignupResponse>
    create_user(const schemas::SignupRequest& request,
                std::string& error_code_out);

    /// 로그인 — bcrypt 비교 후 JWT 발급
    static std::optional<schemas::LoginResponse>
    authenticate(const schemas::LoginRequest& request,
                 std::string& error_code_out);

    /// JWT 발급 (HS256)
    static std::string issue_jwt(const std::string& user_id, int expire_seconds);

    /// JWT 검증 → user_id 반환 (실패 시 std::nullopt)
    static std::optional<std::string> verify_jwt(const std::string& token);

    /// 토큰 무효화 — 블랙리스트 등록 또는 짧은 만료
    static void invalidate_token(const std::string& token);

    /// HTTP Authorization 헤더에서 user_id 추출 (라우터에서 호출)
    static std::optional<std::string> extract_user_id_from_header(const std::string& auth_header);

    // ----- 비밀번호 -----
    static std::string hash_password(const std::string& plain_password);
    static bool verify_password(const std::string& plain_password,
                                const std::string& hashed);
};

} // namespace medibridge::services
