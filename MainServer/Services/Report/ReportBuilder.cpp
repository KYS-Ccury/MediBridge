#include "ReportBuilder.h"
#include "../../Database/Connection.h"
#include "../Pdma/DrugOverviewCache.h"

namespace medibridge::services::report {

schemas::ReportResponse
ReportBuilder::build(const std::string& user_id,
                     const std::string& from_date,
                     const std::string& to_date)
{
    schemas::ReportResponse report;
    report.period.from_date = from_date;
    report.period.to_date = to_date;

    // TODO (영역 B 분담):
    //   1. SELECT FROM medication_intake_logs WHERE user_id = ? AND ...
    //   2. consumed_summary 통계 계산 (약별 개수·기간)
    //   3. 약별 e약은요 → side_effect_quotes (LLM 변환 ❌)
    //   4. intake_logs 채움
    //   5. report_id, generated_at 자동 부여

    return report;
}

std::vector<schemas::SideEffectQuote>
ReportBuilder::collect_side_effect_quotes(const std::vector<std::string>& item_codes)
{
    std::vector<schemas::SideEffectQuote> quotes;

    // TODO (영역 B 분담):
    //   각 item_code 에 대해:
    //     - pdma::DrugOverviewCache::get_or_fetch(item_code)
    //     - caution_text, side_effect_text 그대로 인용
    //     - source_note 통일 문구 첨부 (비단정·상담 권유)
    return quotes;
}

} // namespace medibridge::services::report
