#include "JwtIssuer.h"
#include "../../Config.h"

namespace medibridge::services::auth {

std::string JwtIssuer::issue(const std::string& user_id, int expire_seconds)
{
    // TODO (영역 B 분담):
    //   #include <jwt-cpp/jwt.h>
    //   auto token = jwt::create()
    //       .set_issuer("medibridge")
    //       .set_subject(user_id)
    //       .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(expire_seconds))
    //       .sign(jwt::algorithm::hs256{Config::instance().jwt_secret()});
    return "TODO_JWT_TOKEN";
}

std::optional<std::string> JwtIssuer::verify(const std::string& token)
{
    // TODO: 서명·만료·블랙리스트 검증 → user_id 추출
    return std::nullopt;
}

void JwtIssuer::invalidate(const std::string& token)
{
    // TODO: 블랙리스트 등록 (Redis 또는 메모리 캐시)
}

std::optional<std::string>
JwtIssuer::extract_user_id_from_header(const std::string& auth_header)
{
    // TODO: "Bearer <token>" 파싱 → verify
    return std::nullopt;
}

} // namespace medibridge::services::auth
