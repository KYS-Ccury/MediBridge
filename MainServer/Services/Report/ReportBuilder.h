// =====================================================
// ReportBuilder — 보고서 데이터 수집 (DB + 식약처 e약은요 인용)
// =====================================================
// ⚠️ 부작용 표기는 식약처 e약은요 그대로 인용. LLM 자연어 변환 ❌.
// =====================================================
#pragma once

#include <string>
#include <vector>
#include "../../Schemas/ReportSchema.h"

namespace medibridge::services::report {

class ReportBuilder
{
public:
    /// 지정 기간의 보고서 데이터 수집
    static schemas::ReportResponse
    build(const std::string& user_id,
          const std::string& from_date,
          const std::string& to_date);

private:
    /// 약별 부작용 식약처 인용 (LLM 변환 X)
    static std::vector<schemas::SideEffectQuote>
    collect_side_effect_quotes(const std::vector<std::string>& item_codes);
};

} // namespace medibridge::services::report
