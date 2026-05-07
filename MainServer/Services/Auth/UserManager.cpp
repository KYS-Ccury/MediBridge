// =====================================================
// UserManager — 사용자 CRUD (회원가입·조회) — 실 DB 동작
// =====================================================
#include "UserManager.h"
#include "PasswordHasher.h"
#include "../../Database/Connection.h"
#include "../../Utils/TimeUtil.h"

#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>

namespace medibridge::services::auth {

namespace {

std::string gen_user_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << "user_" << std::hex << gen();
    return ss.str();
}

std::string gen_anonymous_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << "anon_" << std::hex << gen() << gen();
    return ss.str().substr(0, 64);
}

} // anonymous

std::optional<schemas::SignupResponse>
UserManager::create_user(const schemas::SignupRequest& request,
                         std::string& error_code_out)
{
    auto db = database::Connection::instance().client();
    if (!db) {
        error_code_out = "DB_UNAVAILABLE";
        return std::nullopt;
    }

    // 1) 이메일 중복 확인
    if (find_by_email(request.email).has_value()) {
        error_code_out = "EMAIL_TAKEN";
        return std::nullopt;
    }

    // 2) user_id / anonymous_id 생성 + 해시 (모두 prepared statement 로 바인딩)
    const std::string user_id = gen_user_id();
    const std::string anon_id = gen_anonymous_id();
    const std::string pw_hash = PasswordHasher::hash(request.password);

    try {
        db->execSqlSync(
            "INSERT INTO users (user_id, email, password_hash, user_name) VALUES (?, ?, ?, ?)",
            user_id, request.email, pw_hash, request.user_name);
        db->execSqlSync(
            "INSERT INTO pseudonym_map (anonymous_id, user_id) VALUES (?, ?)",
            anon_id, user_id);

        auto rows = db->execSqlSync(
            "SELECT created_at FROM users WHERE user_id = ?", user_id);

        schemas::SignupResponse resp;
        resp.user_id    = user_id;
        resp.email      = request.email;
        resp.user_name  = request.user_name;
        resp.created_at = rows.empty() ? utils::current_iso8601_utc()
                                       : rows[0]["created_at"].as<std::string>();
        return resp;
    } catch (const drogon::orm::DrogonDbException& e) {
        error_code_out = "DB_ERROR";
        return std::nullopt;
    }
}

std::optional<database::models::User>
UserManager::find_by_email(const std::string& email)
{
    auto db = database::Connection::instance().client();
    if (!db) return std::nullopt;
    try {
        auto rows = db->execSqlSync(
            "SELECT user_id, email, password_hash, user_name, created_at "
            "FROM users WHERE email = ? LIMIT 1",
            email);
        if (rows.empty()) return std::nullopt;
        auto row = rows[0];
        database::models::User u;
        u.user_id       = row["user_id"].as<std::string>();
        u.email         = row["email"].as<std::string>();
        u.password_hash = row["password_hash"].as<std::string>();
        u.user_name     = row["user_name"].as<std::string>();
        u.created_at    = row["created_at"].as<std::string>();
        return u;
    } catch (const drogon::orm::DrogonDbException&) {
        return std::nullopt;
    }
}

std::optional<database::models::User>
UserManager::find_by_id(const std::string& user_id)
{
    auto db = database::Connection::instance().client();
    if (!db) return std::nullopt;
    try {
        auto rows = db->execSqlSync(
            "SELECT user_id, email, password_hash, user_name, created_at "
            "FROM users WHERE user_id = ? LIMIT 1",
            user_id);
        if (rows.empty()) return std::nullopt;
        auto row = rows[0];
        database::models::User u;
        u.user_id       = row["user_id"].as<std::string>();
        u.email         = row["email"].as<std::string>();
        u.password_hash = row["password_hash"].as<std::string>();
        u.user_name     = row["user_name"].as<std::string>();
        u.created_at    = row["created_at"].as<std::string>();
        return u;
    } catch (const drogon::orm::DrogonDbException&) {
        return std::nullopt;
    }
}

} // namespace medibridge::services::auth
