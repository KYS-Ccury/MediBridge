#include "UserManager.h"
#include "PasswordHasher.h"
#include "../../Database/Connection.h"

namespace medibridge::services::auth {

std::optional<schemas::SignupResponse>
UserManager::create_user(const schemas::SignupRequest& request,
                         std::string& error_code_out)
{
    // TODO (영역 B 분담) — ⚠ SQL 인젝션 방어 절대 준수:
    //
    // ✅ Drogon ORM (자동 prepared statement):
    //   auto db = drogon::app().getDbClient();
    //   auto future = db->execSqlAsyncFuture(
    //       "INSERT INTO users (user_id, email, password_hash, user_name, created_at) "
    //       "VALUES ($1, $2, $3, $4, NOW())",
    //       user_id, request.email,
    //       PasswordHasher::hash(request.password),     // ← 평문 X, bcrypt 해시만
    //       request.user_name);                          // ← 모두 바인딩 ($1, $2, $3, $4)
    //
    // ❌ 절대 금지 — 문자열 결합:
    //   std::string sql = "INSERT INTO users VALUES ('" + request.email + "', ...)";  // SQL Injection!
    //
    // 흐름:
    //   1. find_by_email() — 중복 시 error_code_out = "EMAIL_ALREADY_EXISTS"
    //   2. user_id 생성 (drogon::utils::getUuid() 또는 별도 uuid 라이브러리)
    //   3. password_hash = PasswordHasher::hash(request.password)
    //   4. INSERT (위 prepared statement)
    //   5. SignupResponse 채워서 반환
    error_code_out = "NOT_IMPLEMENTED";
    return std::nullopt;
}

std::optional<database::models::User>
UserManager::find_by_email(const std::string& email)
{
    // TODO — ✅ 파라미터 바인딩 필수:
    //   db->execSqlAsyncFuture(
    //       "SELECT user_id, email, password_hash, user_name, created_at "
    //       "FROM users WHERE email = $1 LIMIT 1",
    //       email);   // ← 바인딩 (자동 escape)
    return std::nullopt;
}

std::optional<database::models::User>
UserManager::find_by_id(const std::string& user_id)
{
    // TODO — ✅ 파라미터 바인딩 필수:
    //   db->execSqlAsyncFuture(
    //       "SELECT ... FROM users WHERE user_id = $1 LIMIT 1",
    //       user_id);
    return std::nullopt;
}

} // namespace medibridge::services::auth
