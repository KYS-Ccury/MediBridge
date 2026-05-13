// =====================================================
// StorageManager — 파일 디스크 I/O
// =====================================================
// 디렉터리 구조: <storage_root>/<anonymous_id>/<photo_id>.<ext>
//
// 핵심:
//   - put_atomic — <photo_id>.tmp 에 쓴 뒤 rename (반쪽 파일 방지)
//   - get        — 파일 읽어 std::string 으로 반환 (Drogon HttpResponse 가 본문 소유)
//   - exists     — 존재 확인 (Vision PC HEAD 검증용 — 향후)
//
// 모든 함수는 throws X — bool / optional 반환으로 실패 명시.
// =====================================================
#pragma once

#include <string>
#include <optional>

namespace datastorage::services {

class StorageManager
{
public:
    /// 토큰 검증 후 호출. 데이터를 anonymous_id/photo_id.ext 에 atomic 저장.
    /// 성공 시 true, 실패 시 false (err_msg 채워짐).
    static bool put_atomic(const std::string& anonymous_id,
                           const std::string& photo_id,
                           const std::string& ext,
                           const std::string& body,
                           std::string&       err_msg);

    /// 파일 읽어 본문 반환. 없으면 std::nullopt.
    static std::optional<std::string>
    get(const std::string& anonymous_id,
        const std::string& photo_id,
        const std::string& ext);

    /// 존재 여부만 확인 (가벼움)
    static bool exists(const std::string& anonymous_id,
                       const std::string& photo_id,
                       const std::string& ext);

    /// anonymous_id / photo_id / ext 가 안전한 형식인지 화이트리스트 검증
    /// (경로 traversal 방어 — `..`, `/`, 등 차단)
    static bool is_safe(const std::string& id);
    static bool is_allowed_ext(const std::string& ext);  // jpg / png 만
};

} // namespace datastorage::services
