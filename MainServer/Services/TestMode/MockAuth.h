// =====================================================
// MockAuth — TestMode 단계의 mock JWT 발급/검증 헬퍼
// =====================================================
// 운영 모드에서는 사용 금지. Services/Auth/JwtIssuer 가 실 JWT 처리.
// 본 헬퍼는 bcrypt·jwt-cpp 미설치 환경에서 클라가 정상 흐름을 체험할 수
// 있게 하는 단순 토큰 형식만 제공.
//
// 형식: "MEDIBRIDGE_TEST.<base64url(JSON payload)>.SIG"
//   - SIG 는 단순 sha256(jwt_secret + payload) 의 base64url 8자만 검증
//   - 본 모드는 production 에서 강제 비활성 (Config::test_mode 검사)
// =====================================================
#pragma once

#include <string>
#include <optional>

namespace medibridge::testmode {

/// user_id 를 담은 mock JWT 발급
std::string issue_mock_jwt(const std::string& user_id);

/// "Authorization: Bearer <token>" 헤더에서 user_id 추출
/// 실패 시 std::nullopt
std::optional<std::string> extract_user_id_from_bearer(const std::string& auth_header);

} // namespace medibridge::testmode
