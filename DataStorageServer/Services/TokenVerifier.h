// =====================================================
// TokenVerifier — 메인서버 발급 PUT/GET 토큰 검증
// =====================================================
// MainServer/Services/Media/StorageTokenIssuer 와 동일 알고리즘.
// 메인은 issue, 보관 PC 는 verify 만 한다 (시크릿 공유).
//
// 검증 항목 (모두 실패 시 std::nullopt):
//   1. JWT 3-segment 파싱
//   2. HMAC-SHA256 서명 일치 (constant-time)
//   3. iss="medibridge-main", aud="datastorage"
//   4. exp > now (만료 안 됨)
//   5. op == expected_op
//   6. sub, jti, mime, max 모두 존재
//
// 추가 검증은 호출자가 (e.g., URL 경로의 sub/jti 와 토큰 일치 확인).
// =====================================================
#pragma once

#include <string>
#include <optional>

namespace datastorage::services {

struct VerifiedClaims {
    std::string anonymous_id;   // sub
    std::string photo_id;       // jti
    std::string operation;      // op
    std::string mime_type;      // mime
    long        max_bytes = 0;
    long long   expires_at = 0;
};

class TokenVerifier
{
public:
    /// 토큰 검증. expected_op = "put" | "get".
    /// 실패 시 std::nullopt 반환 (실패 사유는 nullopt — 외부 노출 시 정보 누수 방지).
    static std::optional<VerifiedClaims>
    verify(const std::string& token, const std::string& expected_op);

    /// "Authorization: Bearer <token>" 헤더에서 토큰 추출 후 검증
    static std::optional<VerifiedClaims>
    verify_header(const std::string& auth_header, const std::string& expected_op);
};

} // namespace datastorage::services
