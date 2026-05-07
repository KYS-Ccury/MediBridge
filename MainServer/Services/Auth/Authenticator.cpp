// =====================================================
// Authenticator — 로그인 비교 + JWT 발급 (UserManager + PasswordHasher + JwtIssuer 조합)
// =====================================================
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
    auto user_opt = UserManager::find_by_email(request.email);
    if (!user_opt) {
        error_code_out = "INVALID_CREDENTIALS";
        return std::nullopt;
    }
    const auto& user = *user_opt;
    if (!PasswordHasher::verify(request.password, user.password_hash)) {
        error_code_out = "INVALID_CREDENTIALS";
        return std::nullopt;
    }

    schemas::LoginResponse resp;
    resp.access_token   = JwtIssuer::issue(user.user_id,
                                           Config::instance().jwt_expire_seconds());
    resp.token_type     = "Bearer";
    resp.expires_in     = Config::instance().jwt_expire_seconds();
    resp.user.user_id   = user.user_id;
    resp.user.email     = user.email;
    resp.user.user_name = user.user_name;
    return resp;
}

} // namespace medibridge::services::auth
