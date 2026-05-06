#include "Authenticator.h"
#include "UserManager.h"
#include "PasswordHasher.h"
#include "JwtIssuer.h"
#include "../../Config.h"

namespace medibridge::services::auth {

std::optional<schemas::LoginResponse>
Authenticator::authenticate(const schemas::LoginRequest& request,
                            std::string& error_code_out)
{
    // TODO (영역 B 분담):
    //   1. user = UserManager::find_by_email(request.email)
    //      없으면 INVALID_CREDENTIALS
    //   2. PasswordHasher::verify(request.password, user.password_hash)
    //      false 면 INVALID_CREDENTIALS
    //   3. token = JwtIssuer::issue(user.user_id, Config::jwt_expire_seconds())
    //   4. LoginResponse 구성
    error_code_out = "NOT_IMPLEMENTED";
    return std::nullopt;
}

} // namespace medibridge::services::auth
