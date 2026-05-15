// =====================================================
// DrugOverviewCache — e약은요 lazy 캐싱 (TTL 30일, 동기 refresh)
// =====================================================
// 1차 DB 조회 + stale 검사:
//   - DB miss       → 외부 API → UPSERT → 응답
//   - DB hit fresh  → DB 응답
//   - DB hit stale  → 외부 API → UPSERT → 응답  ⭐ (TTL 30일 회귀)
//
// TTL 기본 30일, 환경변수 MEDIBRIDGE_PDMA_CACHE_TTL_DAYS 로 변경 가능.
// 0 설정 시 TTL 비활성 (무기한 캐싱).
// =====================================================
#pragma once

#include <json/json.h>
#include <optional>
#include <string>

namespace medibridge::services::pdma {

class DrugOverviewCache
{
public:
    /// 1차 DB 조회 → (미스 또는 stale) 시 외부 API → DB UPSERT → 반환
    static std::optional<Json::Value>
    get_or_fetch(const std::string& item_code);

    /// 캐시에 누적 저장 (외부 API 응답 후)
    static void cache(const std::string& item_code, const Json::Value& data);

private:
    /// DB만 조회 (외부 호출 없음). second 는 cached_at 으로부터 경과한 일수.
    static std::optional<std::pair<Json::Value, int>>
    get_from_db_with_age(const std::string& item_code);
};

} // namespace medibridge::services::pdma
