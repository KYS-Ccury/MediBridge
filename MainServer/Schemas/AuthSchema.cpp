#include "AuthSchema.h"

namespace medibridge::schemas {

SignupRequest SignupRequest::from_json(const Json::Value& json)
{
    SignupRequest r;
    r.email     = json.get("email", "").asString();
    r.password  = json.get("password", "").asString();
    r.user_name = json.get("user_name", "").asString();
    return r;
}

bool SignupRequest::is_valid(std::string& f, std::string& c) const
{
    if (email.empty())     { f = "email";     c = "MISSING_FIELD"; return false; }
    if (email.find('@') == std::string::npos) { f = "email"; c = "INVALID_EMAIL"; return false; }
    if (password.size() < 8) { f = "password"; c = "WEAK_PASSWORD"; return false; }
    if (user_name.empty())   { f = "user_name"; c = "MISSING_FIELD"; return false; }
    return true;
}

Json::Value SignupResponse::to_json() const
{
    Json::Value v;
    v["user_id"]    = user_id;
    v["email"]      = email;
    v["user_name"]  = user_name;
    v["created_at"] = created_at;
    return v;
}

LoginRequest LoginRequest::from_json(const Json::Value& json)
{
    LoginRequest r;
    r.email    = json.get("email", "").asString();
    r.password = json.get("password", "").asString();
    return r;
}

bool LoginRequest::is_valid(std::string& f, std::string& c) const
{
    if (email.empty())    { f = "email";    c = "MISSING_FIELD"; return false; }
    if (password.empty()) { f = "password"; c = "MISSING_FIELD"; return false; }
    return true;
}

Json::Value UserSummary::to_json() const
{
    Json::Value v;
    v["user_id"]   = user_id;
    v["email"]     = email;
    v["user_name"] = user_name;
    return v;
}

Json::Value LoginResponse::to_json() const
{
    Json::Value v;
    v["access_token"] = access_token;
    v["token_type"]   = token_type;
    v["expires_in"]   = expires_in;
    v["user"]         = user.to_json();
    return v;
}

} // namespace medibridge::schemas
