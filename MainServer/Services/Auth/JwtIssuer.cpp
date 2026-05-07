// =====================================================
// JwtIssuer — JWT (HS256) 발급/검증 — OpenSSL HMAC, jwt-cpp 미사용
// =====================================================
// 형식: header.payload.signature (RFC 7519, RFC 7515)
//   header  = base64url({"alg":"HS256","typ":"JWT"})
//   payload = base64url({"iss":"medibridge","sub":<user_id>,"iat":<now>,"exp":<now+ttl>})
//   sig     = base64url(HMAC-SHA256(header + "." + payload, jwt_secret))
//
// invalidate(): 메모리 블랙리스트 (프로세스 재시작 시 손실 — Redis 또는 만료 단축으로 보강).
// =====================================================
#include "JwtIssuer.h"
#include "../../Config.h"

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <json/json.h>

#include <chrono>
#include <cstring>
#include <mutex>
#include <sstream>
#include <unordered_set>

namespace medibridge::services::auth {

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

std::mutex g_blacklist_mu;
std::unordered_set<std::string> g_blacklist;

bool is_blacklisted(const std::string& token)
{
    std::lock_guard<std::mutex> lk(g_blacklist_mu);
    return g_blacklist.count(token) > 0;
}

} // anonymous

std::string JwtIssuer::issue(const std::string& user_id, int expire_seconds)
{
    const auto now    = std::chrono::system_clock::now();
    const auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    const auto exp_ts = now_ts + expire_seconds;

    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::Value payload;
    payload["iss"] = "medibridge";
    payload["sub"] = user_id;
    payload["iat"] = static_cast<Json::Int64>(now_ts);
    payload["exp"] = static_cast<Json::Int64>(exp_ts);

    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    const std::string h_b64 = b64url_encode(Json::writeString(b, header));
    const std::string p_b64 = b64url_encode(Json::writeString(b, payload));
    const std::string signing_input = h_b64 + "." + p_b64;
    const std::string sig = hmac_sha256(Config::instance().jwt_secret(), signing_input);
    const std::string sig_b64 = b64url_encode(
        reinterpret_cast<const unsigned char*>(sig.data()), sig.size());
    return signing_input + "." + sig_b64;
}

std::optional<std::string> JwtIssuer::verify(const std::string& token)
{
    if (is_blacklisted(token)) return std::nullopt;

    const auto p1 = token.find('.');
    if (p1 == std::string::npos) return std::nullopt;
    const auto p2 = token.find('.', p1 + 1);
    if (p2 == std::string::npos) return std::nullopt;

    const std::string h_b64      = token.substr(0, p1);
    const std::string p_b64      = token.substr(p1 + 1, p2 - p1 - 1);
    const std::string sig_b64    = token.substr(p2 + 1);
    const std::string signing_in = h_b64 + "." + p_b64;

    // 서명 검증
    const std::string expected_sig = hmac_sha256(Config::instance().jwt_secret(), signing_in);
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

    if (!root.isMember("sub") || !root["sub"].isString()) return std::nullopt;

    if (root.isMember("exp") && root["exp"].isInt64()) {
        const auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (now_ts >= root["exp"].asInt64()) return std::nullopt;
    }
    if (root.isMember("iss") && root["iss"].isString()
        && root["iss"].asString() != "medibridge")
    {
        return std::nullopt;
    }
    return root["sub"].asString();
}

void JwtIssuer::invalidate(const std::string& token)
{
    std::lock_guard<std::mutex> lk(g_blacklist_mu);
    g_blacklist.insert(token);
    // TODO: 운영 환경 — Redis 등 외부 저장소 + TTL=토큰 만료시각 적용 권장
}

std::optional<std::string>
JwtIssuer::extract_user_id_from_header(const std::string& auth_header)
{
    static const std::string kPrefix = "Bearer ";
    if (auth_header.size() <= kPrefix.size())             return std::nullopt;
    if (auth_header.compare(0, kPrefix.size(), kPrefix))  return std::nullopt;
    return verify(auth_header.substr(kPrefix.size()));
}

} // namespace medibridge::services::auth
