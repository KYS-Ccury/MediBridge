// =====================================================
// Auth Router — TestMode 단계 구현
// =====================================================
// TestMode: 평문 비교 fallback 허용 (bcrypt 미구현 단계).
// Production: PasswordHasher (PBKDF2/bcrypt) + JwtIssuer (HS256, jwt-cpp 등).
// =====================================================
#include "Auth.h"
#include "../Schemas/AuthSchema.h"
#include "../Database/Connection.h"
#include "../Config.h"
#include "../Services/TestMode/MockAuth.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>
#include <iomanip>

using medibridge::database::Connection;
using medibridge::Config;
namespace TestMode = medibridge::testmode;
namespace orm = drogon::orm;

namespace medibridge::routers {

static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message,
                                              const std::string& field = "")
{
    Json::Value body, err;
    err["code"]    = err_code;
    err["message"] = message;
    if (!field.empty()) {
        Json::Value details; details["field"] = field;
        err["details"] = details;
    }
    body["error"] = err;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

static std::string gen_user_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << "user_" << std::hex << gen();
    return ss.str();
}

static std::string gen_anonymous_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << "anon_" << std::hex << gen() << gen();
    return ss.str().substr(0, 64);
}

/// TestMode: 단순 prefix 해시. Production: bcrypt/pbkdf2 교체.
static std::string mock_hash(const std::string& password)
{
    return "$pbkdf2$test_hash_placeholder$" + password;
}

static bool verify_password(const std::string& stored_hash, const std::string& password)
{
    static const std::string kPrefix = "$pbkdf2$test_hash_placeholder$";
    if (stored_hash.size() > kPrefix.size()
        && stored_hash.compare(0, kPrefix.size(), kPrefix) == 0)
    {
        return stored_hash.substr(kPrefix.size()) == password;
    }
    // TODO: bcrypt/pbkdf2 검증 (Services/Auth/PasswordHasher)
    return false;
}

void Auth::handle_signup(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::SignupRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드/형식 검증 실패", err_field));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        auto exist = db->execSqlSync(
            "SELECT user_id FROM users WHERE email = ? LIMIT 1", reqobj.email);
        if (!exist.empty()) {
            callback(error_response(drogon::k409Conflict, "EMAIL_TAKEN", "이미 사용 중인 이메일입니다.", "email"));
            return;
        }

        const std::string user_id = gen_user_id();
        const std::string anon_id = gen_anonymous_id();
        const std::string pw_hash = mock_hash(reqobj.password);

        db->execSqlSync(
            "INSERT INTO users (user_id, email, password_hash, user_name) VALUES (?, ?, ?, ?)",
            user_id, reqobj.email, pw_hash, reqobj.user_name);

        db->execSqlSync(
            "INSERT INTO pseudonym_map (anonymous_id, user_id) VALUES (?, ?)",
            anon_id, user_id);

        auto rows = db->execSqlSync(
            "SELECT created_at FROM users WHERE user_id = ?", user_id);

        schemas::SignupResponse resp;
        resp.user_id    = user_id;
        resp.email      = reqobj.email;
        resp.user_name  = reqobj.user_name;
        resp.created_at = rows.empty() ? utils::current_iso8601_utc()
                                       : rows[0]["created_at"].as<std::string>();

        auto http = drogon::HttpResponse::newHttpJsonResponse(resp.to_json());
        http->setStatusCode(drogon::k201Created);
        callback(http);
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

void Auth::handle_login(const drogon::HttpRequestPtr& req,
                        std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::LoginRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드 누락", err_field));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        auto rows = db->execSqlSync(
            "SELECT user_id, email, password_hash, user_name FROM users WHERE email = ? LIMIT 1",
            reqobj.email);
        if (rows.empty()) {
            callback(error_response(drogon::k401Unauthorized, "INVALID_CREDENTIALS",
                                    "이메일 또는 비밀번호가 올바르지 않습니다."));
            return;
        }
        auto row = rows[0];
        if (!verify_password(row["password_hash"].as<std::string>(), reqobj.password)) {
            callback(error_response(drogon::k401Unauthorized, "INVALID_CREDENTIALS",
                                    "이메일 또는 비밀번호가 올바르지 않습니다."));
            return;
        }

        // TestMode 토큰 발급. 운영 모드는 JwtIssuer 로 교체 (TODO).
        const std::string user_id = row["user_id"].as<std::string>();
        const std::string token   = TestMode::issue_mock_jwt(user_id);

        schemas::LoginResponse resp;
        resp.access_token = token;
        resp.token_type   = "Bearer";
        resp.expires_in   = Config::instance().jwt_expire_seconds();
        resp.user.user_id   = user_id;
        resp.user.email     = row["email"].as<std::string>();
        resp.user.user_name = row["user_name"].as<std::string>();

        callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

void Auth::handle_logout(const drogon::HttpRequestPtr& /*req*/,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // 본 단계: 만료에 의존. 추후 토큰 블랙리스트(Redis 등) 도입 시 검증 추가.
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k204NoContent);
    callback(resp);
}

} // namespace medibridge::routers
