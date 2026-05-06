#include "UserManager.h"
#include "PasswordHasher.h"
#include "../../Database/Connection.h"

namespace medibridge::services::auth {

std::optional<schemas::SignupResponse>
UserManager::create_user(const schemas::SignupRequest& request,
                         std::string& error_code_out)
{
    // TODO (영역 B 분담):
    //   1. find_by_email() — 중복 시 EMAIL_ALREADY_EXISTS
    //   2. user_id 생성 (UUID v4)
    //   3. password_hash = PasswordHasher::hash(request.password)
    //   4. INSERT INTO users (...) — 파라미터 바인딩
    //   5. SignupResponse 채워서 반환
    error_code_out = "NOT_IMPLEMENTED";
    return std::nullopt;
}

std::optional<database::models::User>
UserManager::find_by_email(const std::string& email)
{
    // TODO: SELECT * FROM users WHERE email = ?
    return std::nullopt;
}

std::optional<database::models::User>
UserManager::find_by_id(const std::string& user_id)
{
    // TODO: SELECT * FROM users WHERE user_id = ?
    return std::nullopt;
}

} // namespace medibridge::services::auth
