// =====================================================
// AuthSchema — Auth API 요청/응답 데이터 형식
// =====================================================
// AuthApi.md 의 JSON 형식을 C++ struct로 표현.
// Drogon Json::Value 와 변환.
// =====================================================
#pragma once

#include <json/json.h>
#include <string>

namespace medibridge::schemas {

// ----- POST /v1/auth/signup -----
struct SignupRequest {
    std::string email;
    std::string password;
    std::string user_name;

    static SignupRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct SignupResponse {
    std::string user_id;
    std::string email;
    std::string user_name;
    std::string created_at;   // ISO 8601

    Json::Value to_json() const;
};

// ----- POST /v1/auth/login -----
struct LoginRequest {
    std::string email;
    std::string password;

    static LoginRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct UserSummary {
    std::string user_id;
    std::string email;
    std::string user_name;

    Json::Value to_json() const;
};

struct LoginResponse {
    std::string access_token;
    std::string token_type;        // 항상 "Bearer"
    int         expires_in;        // 초 단위
    UserSummary user;

    Json::Value to_json() const;
};

// ----- POST /v1/auth/logout -----
// 요청 바디 없음. 응답 204.

} // namespace medibridge::schemas
