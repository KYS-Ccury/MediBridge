// =====================================================
// PdmaCacheManager — 식약처 데이터 캐시 관리
// =====================================================
// 정책 (DB ERD v2 / 시스템 흐름 v2):
//   - 낱알식별·DUR 품목·DUR 성분: 일괄 적재 (CSV/EXCEL 또는 스마트싱크)
//     → 운용 중 외부 API 호출 없음
//   - e약은요: lazy 캐싱 (요청 시점에 조회 후 DB 누적, TTL 정책 없음)
// =====================================================
#pragma once

#include <string>
#include <optional>
#include <json/json.h>

namespace medibridge::services {

class PdmaCacheManager
{
public:
    /// 낱알식별 정보 조회 (DB만 — 운용 중 외부 호출 없음)
    static std::optional<Json::Value>
    get_pill_identification(const std::string& item_code);

    /// e약은요 조회 (1차 DB → 캐시 미스 시 외부 API 호출 후 누적)
    static std::optional<Json::Value>
    get_drug_overview(const std::string& item_code);

    /// 식약처 외부 API 호출 (e약은요만) — lazy 캐싱 시에만 호출됨
    static std::optional<Json::Value>
    fetch_drug_overview_from_pdma(const std::string& item_code);

    /// 캐시에 누적 저장
    static void cache_drug_overview(const std::string& item_code,
                                    const Json::Value& data);
};

} // namespace medibridge::services
