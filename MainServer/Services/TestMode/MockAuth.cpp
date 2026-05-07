#include "MockAuth.h"

#include <json/json.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cstring>

#include "../../Config.h"

namespace medibridge::testmode {

// ----- base64url (no padding) -----
static const char kAlphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

static std::string base64url_encode(const std::string& in)
{
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 3 <= in.size(); i += 3) {
        uint32_t n = (static_cast<uint8_t>(in[i])     << 16)
                   | (static_cast<uint8_t>(in[i + 1]) <<  8)
                   |  static_cast<uint8_t>(in[i + 2]);
        out.push_back(kAlphabet[(n >> 18) & 0x3F]);
        out.push_back(kAlphabet[(n >> 12) & 0x3F]);
        out.push_back(kAlphabet[(n >>  6) & 0x3F]);
        out.push_back(kAlphabet[ n        & 0x3F]);
    }
    if (i < in.size()) {
        uint32_t n = static_cast<uint8_t>(in[i]) << 16;
        if (i + 1 < in.size()) n |= static_cast<uint8_t>(in[i + 1]) << 8;
        out.push_back(kAlphabet[(n >> 18) & 0x3F]);
        out.push_back(kAlphabet[(n >> 12) & 0x3F]);
        if (i + 1 < in.size())
            out.push_back(kAlphabet[(n >> 6) & 0x3F]);
    }
    return out;
}

static int b64idx(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

static std::string base64url_decode(const std::string& in)
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

// ----- 시크릿 + payload 를 sha256 → base64url 8자 -----
static std::string short_sig(const std::string& secret, const std::string& payload_b64)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    std::string mat = secret + "." + payload_b64;
    SHA256(reinterpret_cast<const unsigned char*>(mat.data()), mat.size(), digest);
    std::string raw(reinterpret_cast<const char*>(digest), 6);  // 6바이트 → base64url 8자
    return base64url_encode(raw);
}

std::string issue_mock_jwt(const std::string& user_id)
{
    Json::Value payload;
    payload["user_id"] = user_id;
    payload["mode"]    = "test";

    Json::StreamWriterBuilder b;
    b["indentation"] = "";
    const std::string body_b64 = base64url_encode(Json::writeString(b, payload));
    const std::string sig      = short_sig(Config::instance().jwt_secret(), body_b64);
    return std::string("MEDIBRIDGE_TEST.") + body_b64 + "." + sig;
}

std::optional<std::string> extract_user_id_from_bearer(const std::string& auth_header)
{
    static const std::string kPrefix = "Bearer ";
    if (auth_header.size() <= kPrefix.size())              return std::nullopt;
    if (auth_header.compare(0, kPrefix.size(), kPrefix))   return std::nullopt;

    const std::string token = auth_header.substr(kPrefix.size());

    // "MEDIBRIDGE_TEST.<payload>.<sig>"
    const auto p1 = token.find('.');
    if (p1 == std::string::npos)              return std::nullopt;
    const auto p2 = token.find('.', p1 + 1);
    if (p2 == std::string::npos)              return std::nullopt;

    const std::string header  = token.substr(0, p1);
    const std::string payload = token.substr(p1 + 1, p2 - p1 - 1);
    const std::string sig     = token.substr(p2 + 1);

    if (header != "MEDIBRIDGE_TEST")                              return std::nullopt;
    if (sig != short_sig(Config::instance().jwt_secret(), payload)) return std::nullopt;

    const std::string json_text = base64url_decode(payload);
    if (json_text.empty())                                        return std::nullopt;

    Json::CharReaderBuilder rb;
    Json::Value root;
    std::string errs;
    std::istringstream iss(json_text);
    if (!Json::parseFromStream(rb, iss, &root, &errs))            return std::nullopt;

    if (!root.isMember("user_id") || !root["user_id"].isString()) return std::nullopt;
    return root["user_id"].asString();
}

} // namespace medibridge::testmode
