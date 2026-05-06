#include "PasswordHasher.h"

namespace medibridge::services::auth {

std::string PasswordHasher::hash(const std::string& plain_password)
{
    // TODO (영역 B 분담):
    //   - libbcrypt 또는 OpenSSL EVP_PBKDF2 사용
    //   - cost=12 권장
    //   - 평문 저장 절대 금지
    return "TODO_BCRYPT_HASH";
}

bool PasswordHasher::verify(const std::string& plain_password,
                            const std::string& hashed)
{
    // TODO: bcrypt verify
    return false;
}

} // namespace medibridge::services::auth
