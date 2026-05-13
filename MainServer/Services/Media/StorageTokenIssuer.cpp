// =====================================================
// StorageTokenIssuer — 구현 (HS256, JWT 호환 형식)
// =====================================================
// JwtIssuer 와 동일한 base64url + HMAC-SHA256 알고리즘.
// 시크릿만 storage_secret() 으로 분리하고, aud="datastorage" 강제.
//
// JwtIssuer 와 코드 중복이 있지만, 의도적으로 분리:
//   - JWT 시크릿이 유출되어도 storage 토큰 위조 불가 (시크릿 분리)
//   - 향후 storage 토큰 알고리즘 변경 (예: ED25519) 시 영향 격리
// =====================================================
#include "StorageTokenIssuer.h"
#include "../../Config.h"

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <json/json.h>

#include <chrono>
#include <sstream>
#include <cstring>

namespace medibridge::services::media {

namespace {

static const char kAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string b64url_encode(const unsigned char* data, size_t len)
{
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(kAlphabet[(n >> 18) & 0x3F]);
        out.push_back(kAlphabet[(n >> 12) & 0x3F]);
        out.push_back(kAlphabet[(n >>  6) & 0x3F]);
        out.push_back(kAlphabet[ n        & 0x3F]);
    }
    if (i < len) {
        uint32_t n = data[i] << 16;
        if (i + 1 < len) n |= data[i + 1] << 8;
        out.push_back(kAlphabet[(n >> 18) & 0x3F]);
        out.push_back(kAlphabet[(n >> 12) & 0x3F]);
        if (i + 1 < len) out.push_back(kAlphabet[(n >> 6) & 0x3F]);
    }
    return out;
}

inline std::string b64url_encode(const std::string& s)
{
    return b64url_encode(reinterpret_cast<const unsigned char*>(s.data()), s.size());
}

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

std::string StorageTokenIssuer::issue(const StorageTokenClaims& c, int expire_seconds)
{
    const auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const int ttl = (expire_seconds > 0)
        ? expire_seconds
        : Config::instance().storage_token_ttl_seconds();
    const auto exp_ts = now_ts + ttl;

    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::Value payload;
    payload["iss"]  = kIssuer;
    payload["aud"]  = kAudience;
    payload["sub"]  = c.anonymous_id;
    payload["jti"]  = c.photo_id;
    payload["op"]   = c.operation;
    payload["mime"] = c.mime_type;
    payload["max"]  = static_cast<Json::Int64>(c.max_bytes);
    payload["iat"]  = static_cast<Json::Int64>(now_ts);
    payload["exp"]  = static_cast<Json::Int64>(exp_ts);

    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    const std::string h_b64 = b64url_encode(Json::writeString(b, header));
    const std::string p_b64 = b64url_encode(Json::writeString(b, payload));
    const std::string signing_input = h_b64 + "." + p_b64;
    const std::string sig = hmac_sha256(Config::instance().storage_secret(), signing_input);
    const std::string sig_b64 = b64url_encode(
        reinterpret_cast<const unsigned char*>(sig.data()), sig.size());
    return signing_input + "." + sig_b64;
}

std::optional<StorageTokenClaims>
StorageTokenIssuer::verify(const std::string& token, const std::string& expected_op)
{
    const auto p1 = token.find('.');
    if (p1 == std::string::npos) return std::nullopt;
    const auto p2 = token.find('.', p1 + 1);
    if (p2 == std::string::npos) return std::nullopt;

    const std::string h_b64      = token.substr(0, p1);
    const std::string p_b64      = token.substr(p1 + 1, p2 - p1 - 1);
    const std::string sig_b64    = token.substr(p2 + 1);
    const std::string signing_in = h_b64 + "." + p_b64;

    // 서명 검증
    const std::string expected_sig = hmac_sha256(Config::instance().storage_secret(), signing_in);
    const std::string provided_sig = b64url_decode(sig_b64);
    if (provided_sig.empty())                  return std::nullopt;
    if (!ct_equal(expected_sig, provided_sig)) return std::nullopt;

    // payload 파싱
    const std::string payload_json = b64url_decode(p_b64);
    if (payload_json.empty()) return std::nullopt;

    Json::CharReaderBuilder rb;
    Json::Value root;
    std::string errs;
    std::istringstream iss(payload_json);
    if (!Json::parseFromStream(rb, iss, &root, &errs)) return std::nullopt;

    // iss / aud 강제 검증 (JWT 와 토큰 혼용 방어)
    if (!root.isMember("iss") || root["iss"].asString() != kIssuer)   return std::nullopt;
    if (!root.isMember("aud") || root["aud"].asString() != kAudience) return std::nullopt;

    // 필수 필드
    if (!root.isMember("sub") || !root["sub"].isString()) return std::nullopt;
    if (!root.isMember("jti") || !root["jti"].isString()) return std::nullopt;
    if (!root.isMember("op")  || !root["op"].isString())  return std::nullopt;
    if (!root.isMember("exp") || !root["exp"].isInt64())  return std::nullopt;

    // 만료
    const auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const long long exp_ts = root["exp"].asInt64();
    if (now_ts >= exp_ts) return std::nullopt;

    // op 일치
    const std::string op = root["op"].asString();
    if (op != "put" && op != "get") return std::nullopt;
    if (!expected_op.empty() && op != expected_op) return std::nullopt;

    StorageTokenClaims c;
    c.anonymous_id = root["sub"].asString();
    c.photo_id     = root["jti"].asString();
    c.operation    = op;
    c.mime_type    = root.get("mime", "").asString();
    c.max_bytes    = root.get("max", 0).asInt64();
    c.issued_at    = root.get("iat", 0).asInt64();
    c.expires_at   = exp_ts;
    return c;
}

} // namespace medibridge::services::media
