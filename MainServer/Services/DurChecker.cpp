#include "DurChecker.h"
#include "../Database/Connection.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace medibridge::services {

schemas::DurCheckResult DurChecker::check_combination(
    const std::vector<std::string>& item_codes)
{
    schemas::DurCheckResult result;

    // 현재 시각 ISO 8601
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    result.checked_at = ss.str();

    // TODO (영역 B 분담):
    //   1. query_pairwise_risks(item_codes) → 페어 조합별 DUR 조회
    //   2. 결과가 비어있으면 result.result = "no_risk_found"
    //   3. 있으면 result.result = "risk_found" + details 채움
    //   4. 식약처 prohibit_reason 본문은 절대 가공하지 말 것 (그대로 인용)
    result.details = query_pairwise_risks(item_codes);
    result.result = result.details.empty() ? "no_risk_found" : "risk_found";

    return result;
}

std::string DurChecker::format_risk_message(const schemas::DurDetail& d)
{
    // 정해진 템플릿 — 단정 표현 ❌, "즉시 약사·의사 상담 필요" 톤
    std::ostringstream oss;
    oss << "[" << d.dur_type << "]\n"
        << d.drug_a_name << " + " << d.drug_b_name << "\n"
        << "사유: " << d.prohibit_reason << "\n"
        << "조치: 즉시 약사·의사 상담 필요";
    return oss.str();
}

std::string DurChecker::format_safe_message()
{
    return "[안전 안내]\n"
           "DUR에 등록 확인되지 않았습니다.\n"
           "안심을 위해 약사·의사 상담을 권유드립니다.";
}

std::vector<schemas::DurDetail>
DurChecker::query_pairwise_risks(const std::vector<std::string>& item_codes)
{
    // TODO (영역 B 분담):
    //   SELECT dur_id, base_item_code, target_item_code, dur_type, prohibit_reason
    //   FROM dur_interaction_cache
    //   WHERE (base_item_code IN (...) AND target_item_code IN (...))
    //   파라미터 바인딩 필수 (SQL 인젝션 방어)
    return {};
}

} // namespace medibridge::services
