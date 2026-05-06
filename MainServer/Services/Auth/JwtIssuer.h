// =====================================================
// JwtIssuer — JWT 발급·검증·무효화
// =====================================================
#pragma once

#include <string>
#include <optional>

namespace medibridge::services::auth {

class JwtIssuer
{
public:
    /// JWT 발급 (HS256, Config::jwt_secret 사용)
    static std::string issue(const std::string& user_id,
                             int expire_seconds);

    /// JWT 검증 → user_id 반환 (실패 시 std::nullopt)
    static std::optional<std::string> verify(const std::string& token);

    /// 토큰 블랙리스트 등록 (logout 시)
    static void invalidate(const std::string& token);

    /// "Authorization: Bearer <token>" 헤더에서 user_id 추출
    static std::optional<std::string>
    extract_user_id_from_header(const std::string& auth_header);
};

} // namespace medibridge::services::auth
