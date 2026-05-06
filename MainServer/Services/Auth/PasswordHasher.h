// =====================================================
// PasswordHasher — bcrypt 비밀번호 해시·검증 (정적 유틸)
// =====================================================
#pragma once

#include <string>

namespace medibridge::services::auth {

class PasswordHasher
{
public:
    /// 평문 비밀번호 → bcrypt 해시 (cost=12 권장)
    static std::string hash(const std::string& plain_password);

    /// 평문과 해시 비교
    static bool verify(const std::string& plain_password,
                       const std::string& hashed);
};

} // namespace medibridge::services::auth
