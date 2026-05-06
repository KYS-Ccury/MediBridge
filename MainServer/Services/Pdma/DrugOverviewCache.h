// =====================================================
// DrugOverviewCache — e약은요 lazy 캐싱 (TTL 정책 없음)
// =====================================================
// 1차: DB 조회. 캐시 미스 시 PdmaApiClient 통해 외부 API 호출 후 누적.
// =====================================================
#pragma once

#include <json/json.h>
#include <optional>
#include <string>

namespace medibridge::services::pdma {

class DrugOverviewCache
{
public:
    /// 1차 DB 조회 → 미스 시 외부 API 호출 → DB 누적 → 반환
    static std::optional<Json::Value>
    get_or_fetch(const std::string& item_code);

    /// 캐시에 누적 저장 (외부 API 응답 후)
    static void cache(const std::string& item_code, const Json::Value& data);

private:
    /// DB만 조회 (외부 호출 없음)
    static std::optional<Json::Value> get_from_db(const std::string& item_code);
};

} // namespace medibridge::services::pdma
