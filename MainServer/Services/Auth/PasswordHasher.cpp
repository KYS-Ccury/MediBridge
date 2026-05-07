// =====================================================
// PasswordHasher — PBKDF2-SHA256 비밀번호 해시 (OpenSSL only)
// =====================================================
// 형식: $pbkdf2-sha256$<iterations>$<base64(salt)>$<base64(hash)>
//   - iterations: 100,000
//   - salt:       16 byte 랜덤
//   - hash:       32 byte (PBKDF2-HMAC-SHA256 출력)
//
// TestMode 호환:
//   - $pbkdf2$test_hash_placeholder$<plain> 형식은 평문 비교로 통과시킴.
//     production(MEDIBRIDGE_ENV=production)에서는 거부.
// =====================================================
#include "PasswordHasher.h"
#include "../../Config.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace medibridge::services::auth {

namespace {

constexpr int    kIterations  = 100'000;
constexpr size_t kSaltLength  = 16;
constexpr size_t kHashLength  = 32;
constexpr const char* kPrefix     = "$pbkdf2-sha256$";
constexpr const char* kTestPrefix = "$pbkdf2$test_hash_placeholder$";

static const char kB64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const unsigned char* data, size_t len)
{
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(kB64Alphabet[(n >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(n >> 12) & 0x3F]);
        out.push_back(kB64Alphabet[(n >>  6) & 0x3F]);
        out.push_back(kB64Alphabet[ n        & 0x3F]);
    }
    if (i < len) {
        uint32_t n = data[i] << 16;
        if (i + 1 < len) n |= data[i + 1] << 8;
        out.push_back(kB64Alphabet[(n >> 18) & 0x3F]);
        out.push_back(kB64Alphabet[(n >> 12) & 0x3F]);
        out.push_back((i + 1 < len) ? kB64Alphabet[(n >> 6) & 0x3F] : '=');
        out.push_back('=');
    }
    return out;
}

int b64idx(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::vector<unsigned char> base64_decode(const std::string& in)
{
    std::vector<unsigned char> out;
    out.reserve((in.size() * 3) / 4);
    int buf = 0, bits = 0;
    for (char c : in) {
        if (c == '=') break;
        int v = b64idx(c);
        if (v < 0) return {};
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

bool pbkdf2_sha256(const std::string& password,
                   const unsigned char* salt, size_t salt_len,
                   int iterations,
                   unsigned char* out, size_t out_len)
{
    return PKCS5_PBKDF2_HMAC(password.data(),
                             static_cast<int>(password.size()),
                             salt, static_cast<int>(salt_len),
                             iterations,
                             EVP_sha256(),
                             static_cast<int>(out_len),
                             out) == 1;
}

bool constant_time_equal(const std::vector<unsigned char>& a,
                         const std::vector<unsigned char>& b)
{
    if (a.size() != b.size()) return false;
    unsigned char acc = 0;
    for (size_t i = 0; i < a.size(); ++i) acc |= a[i] ^ b[i];
    return acc == 0;
}

bool is_production()
{
    const char* env = std::getenv("MEDIBRIDGE_ENV");
    return env && std::string(env) == "production";
}

} // anonymous

std::string PasswordHasher::hash(const std::string& plain_password)
{
    std::array<unsigned char, kSaltLength> salt{};
    if (RAND_bytes(salt.data(), kSaltLength) != 1) {
        throw std::runtime_error("PasswordHasher: salt 생성 실패");
    }
    std::array<unsigned char, kHashLength> dk{};
    if (!pbkdf2_sha256(plain_password,
                       salt.data(), salt.size(),
                       kIterations,
                       dk.data(), dk.size()))
    {
        throw std::runtime_error("PasswordHasher: PBKDF2 실패");
    }
    std::ostringstream oss;
    oss << kPrefix << kIterations << "$"
        << base64_encode(salt.data(), salt.size()) << "$"
        << base64_encode(dk.data(),   dk.size());
    return oss.str();
}

bool PasswordHasher::verify(const std::string& plain_password,
                            const std::string& hashed)
{
    // ----- TestMode placeholder 형식 -----
    if (hashed.size() > std::strlen(kTestPrefix)
        && hashed.compare(0, std::strlen(kTestPrefix), kTestPrefix) == 0)
    {
        if (is_production()) return false;        // production 거부
        return hashed.substr(std::strlen(kTestPrefix)) == plain_password;
    }

    // ----- 정식 PBKDF2-SHA256 -----
    const std::string prefix = kPrefix;
    if (hashed.size() <= prefix.size()
        || hashed.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    const auto p1 = hashed.find('$', prefix.size());
    if (p1 == std::string::npos) return false;
    const auto p2 = hashed.find('$', p1 + 1);
    if (p2 == std::string::npos) return false;

    int iter = 0;
    try {
        iter = std::stoi(hashed.substr(prefix.size(), p1 - prefix.size()));
    } catch (...) { return false; }
    if (iter < 1000 || iter > 10'000'000) return false;

    const auto salt     = base64_decode(hashed.substr(p1 + 1, p2 - p1 - 1));
    const auto expected = base64_decode(hashed.substr(p2 + 1));
    if (salt.empty() || expected.empty()) return false;
    if (expected.size() != kHashLength)   return false;

    std::array<unsigned char, kHashLength> dk{};
    if (!pbkdf2_sha256(plain_password,
                       salt.data(), salt.size(),
                       iter,
                       dk.data(), dk.size()))
    {
        return false;
    }
    std::vector<unsigned char> dk_v(dk.begin(), dk.end());
    return constant_time_equal(dk_v, expected);
}

} // namespace medibridge::services::auth
