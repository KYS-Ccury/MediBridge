// =====================================================
// StorageTokenIssuer — 데이터 보관 PC 단기 PUT/GET 토큰
// =====================================================
// 메인서버가 발급, 데이터 보관 PC 가 검증.
//   - HS256 (HMAC-SHA256) — Config::storage_secret() 사용 (JWT 시크릿과 별개)
//   - 형식: JWT 호환 (header.payload.signature)
//   - aud="datastorage" 강제 (JWT 와 토큰 혼용 방어)
//   - op ∈ {put, get} — PUT 토큰으로 GET 호출 불가
//   - jti = photo_id (1회 사용 — 보관 PC 측에서 redeemed 추적 권장)
//
// payload 예시:
//   {"iss":"medibridge-main","aud":"datastorage","sub":"anon_test_001",
//    "jti":"ph_8f3a...","op":"put","mime":"image/jpeg","max":10485760,
//    "iat":1715000000,"exp":1715000300}
// =====================================================
#pragma once

#include <string>
#include <optional>

namespace medibridge::services::media {

struct StorageTokenClaims {
    std::string anonymous_id;   // sub
    std::string photo_id;       // jti
    std::string operation;      // op: "put" | "get"
    std::string mime_type;      // mime
    long        max_bytes = 0;  // max
    long long   issued_at = 0;  // iat
    long long   expires_at = 0; // exp
};

class StorageTokenIssuer
{
public:
    /// PUT/GET 토큰 발급. expire_seconds=0 이면 Config 기본값(storage_token_ttl_seconds).
    static std::string issue(const StorageTokenClaims& claims,
                             int expire_seconds = 0);

    /// 토큰 검증. 실패 시 std::nullopt.
    /// expected_op 가 빈 문자열이면 op 비검사 (PUT/GET 양쪽 허용).
    static std::optional<StorageTokenClaims>
    verify(const std::string& token, const std::string& expected_op = "");
};

} // namespace medibridge::services::media
