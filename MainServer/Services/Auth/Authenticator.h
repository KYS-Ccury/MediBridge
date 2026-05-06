// =====================================================
// Authenticator — 로그인 검증 (UserManager + PasswordHasher + JwtIssuer 조합)
// =====================================================
#pragma once

#include <optional>
#include <string>
#include "../../Schemas/AuthSchema.h"

namespace medibridge::services::auth {

class Authenticator
{
public:
    /// 로그인 — bcrypt 비교 후 JWT 발급
    static std::optional<schemas::LoginResponse>
    authenticate(const schemas::LoginRequest& request,
                 std::string& error_code_out);
};

} // namespace medibridge::services::auth
