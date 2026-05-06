#include "DurQueryEngine.h"
#include "../../Database/Connection.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace medibridge::services::dur {

schemas::DurCheckResult DurQueryEngine::check_combination(
    const std::vector<std::string>& item_codes)
{
    schemas::DurCheckResult result;

    // 현재 시각 ISO 8601
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    result.checked_at = ss.str();

    result.details = query_pairwise_risks(item_codes);
    result.result = result.details.empty() ? "no_risk_found" : "risk_found";
    return result;
}

std::vector<schemas::DurDetail>
DurQueryEngine::query_pairwise_risks(const std::vector<std::string>& item_codes)
{
    // TODO (영역 B 분담):
    //   SELECT dur_id, base_item_code, target_item_code, dur_type, prohibit_reason
    //   FROM dur_interaction_cache
    //   WHERE base_item_code IN (...) AND target_item_code IN (...)
    //   파라미터 바인딩 필수 (SQL 인젝션 방어)
    //   ⚠ prohibit_reason 은 식약처 본문 그대로 (가공 X)
    return {};
}

} // namespace medibridge::services::dur
