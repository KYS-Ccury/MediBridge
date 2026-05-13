// =====================================================
// TokenVerifier — 구현 (HMAC-SHA256, JWT 호환)
// =====================================================
// 메인서버 측 StorageTokenIssuer 와 동일한 base64url + HMAC-SHA256.
// =====================================================
#include "TokenVerifier.h"
#include "../Config.h"

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <json/json.h>

#include <chrono>
#include <sstream>

namespace datastorage::services {

namespace {

int b64idx(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

std::string b64url_decode(const std::string& in)
{
    std::string out;
    out.reserve((in.size() * 3) / 4);
    int buf = 0, bits = 0;
    for (char c : in) {
        int v = b64idx(c);
        if (v < 0) return {};
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

std::string hmac_sha256(const std::string& key, const std::string& data)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    HMAC(EVP_sha256(),
         key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         digest, &digest_len);
    return std::string(reinterpret_cast<const char*>(digest), digest_len);
}

/// timing-safe 비교
bool ct_equal(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    unsigned char acc = 0;
    for (size_t i = 0; i < a.size(); ++i) acc |= a[i] ^ b[i];
    return acc == 0;
}

constexpr const char* kIssuer   = "medibridge-main";
constexpr const char* kAudience = "datastorage";

} // anonymous

std::optional<VerifiedClaims>
TokenVerifier::verify(const std::string& token, const std::string& expected_op)
{
    const auto p1 = token.find('.');
    if (p1 == std::string::npos) return std::nullopt;
    const auto p2 = token.find('.', p1 + 1);
    if (p2 == std::string::npos) return std::nullopt;

    const std::string h_b64      = token.substr(0, p1);
    const std::string p_b64      = token.substr(p1 + 1, p2 - p1 - 1);
    const std::string sig_b64    = token.substr(p2 + 1);
    const std::string signing_in = h_b64 + "." + p_b64;

    const std::string expected_sig = hmac_sha256(Config::instance().shared_secret(), signing_in);
    const std::string provided_sig = b64url_decode(sig_b64);
    if (provided_sig.empty())                  return std::nullopt;
    if (!ct_equal(expected_sig, provided_sig)) return std::nullopt;

    const std::string payload_json = b64url_decode(p_b64);
    if (payload_json.empty()) return std::nullopt;

    Json::CharReaderBuilder rb;
    Json::Value root;
    std::string errs;
    std::istringstream iss(payload_json);
    if (!Json::parseFromStream(rb, iss, &root, &errs)) return std::nullopt;

    if (!root.isMember("iss") || root["iss"].asString() != kIssuer)   return std::nullopt;
    if (!root.isMember("aud") || root["aud"].asString() != kAudience) return std::nullopt;
    if (!root.isMember("sub") || !root["sub"].isString()) return std::nullopt;
    if (!root.isMember("jti") || !root["jti"].isString()) return std::nullopt;
    if (!root.isMember("op")  || !root["op"].isString())  return std::nullopt;
    if (!root.isMember("exp") || !root["exp"].isInt64())  return std::nullopt;

    const auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const long long exp_ts = root["exp"].asInt64();
    if (now_ts >= exp_ts) return std::nullopt;

    const std::string op = root["op"].asString();
    if (op != expected_op) return std::nullopt;

    VerifiedClaims c;
    c.anonymous_id = root["sub"].asString();
    c.photo_id     = root["jti"].asString();
    c.operation    = op;
    c.mime_type    = root.get("mime", "").asString();
    c.max_bytes    = root.get("max", 0).asInt64();
    c.expires_at   = exp_ts;
    return c;
}

std::optional<VerifiedClaims>
TokenVerifier::verify_header(const std::string& auth_header, const std::string& expected_op)
{
    static const std::string kPrefix = "Bearer ";
    if (auth_header.size() <= kPrefix.size())             return std::nullopt;
    if (auth_header.compare(0, kPrefix.size(), kPrefix))  return std::nullopt;
    return verify(auth_header.substr(kPrefix.size()), expected_op);
}

} // namespace datastorage::services
