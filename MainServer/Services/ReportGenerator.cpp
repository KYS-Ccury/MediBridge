#include "ReportGenerator.h"
#include "../Database/Connection.h"
#include "PdmaCacheManager.h"

namespace medibridge::services {

schemas::ReportResponse
ReportGenerator::build_report(const std::string& user_id,
                              const std::string& from_date,
                              const std::string& to_date)
{
    schemas::ReportResponse report;
    report.period.from_date = from_date;
    report.period.to_date = to_date;

    // TODO (영역 B 분담):
    //   1. SELECT FROM medication_intake_logs WHERE user_id = ? AND ...
    //   2. consumed_summary 통계 계산 (약별 개수·기간)
    //   3. 약별 e약은요 조회 → side_effect_quotes 채움 (LLM 자연어 변환 ❌)
    //   4. intake_logs 채움
    //   5. report_id, generated_at 자동 부여

    return report;
}

std::string ReportGenerator::render_html(const schemas::ReportResponse& report)
{
    // TODO (영역 B 분담):
    //   1. 인쇄 친화적 HTML 템플릿 (CSS @media print 적용)
    //   2. 사용자 정보·기간·복약 이력 표·부작용 인용 섹션 채움
    //   3. source_note (비단정·상담 권유 안내) 모든 인용에 첨부
    return "<!DOCTYPE html><html><body>TODO Report HTML</body></html>";
}

std::vector<unsigned char> ReportGenerator::render_pdf(const std::string& html)
{
    // TODO (영역 B 분담):
    //   - wkhtmltopdf, libharu, Chromium headless 등 선택
    //   - 무거운 작업이므로 WorkerPool 위임 권장
    return {};
}

std::vector<schemas::SideEffectQuote>
ReportGenerator::collect_side_effect_quotes(const std::vector<std::string>& item_codes)
{
    // TODO: 각 item_code에 대해 PdmaCacheManager::get_drug_overview() 호출
    //   → caution_text, side_effect_text 그대로 인용
    //   → source_note 통일 문구 첨부
    return {};
}

} // namespace medibridge::services
