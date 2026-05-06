// =====================================================
// ReportGenerator — 통합 보고서 생성 (모듈 5)
// =====================================================
// 부작용 표기는 식약처 e약은요 그대로 인용 (LLM 자연어 변환 ❌).
// 무거운 PDF 렌더링은 Threading::WorkerPool 위임.
// =====================================================
#pragma once

#include <string>
#include <vector>
#include "../Schemas/ReportSchema.h"

namespace medibridge::services {

class ReportGenerator
{
public:
    /// 지정 기간의 보고서 데이터 수집 (DB 조회 + e약은요 인용)
    static schemas::ReportResponse
    build_report(const std::string& user_id,
                 const std::string& from_date,
                 const std::string& to_date);

    /// JSON → HTML 렌더링 (인쇄·PDF 변환에 사용)
    static std::string render_html(const schemas::ReportResponse& report);

    /// HTML → PDF 바이너리 (무거운 작업 — WorkerPool 위임 권장)
    static std::vector<unsigned char> render_pdf(const std::string& html);

private:
    /// 약별 부작용 식약처 인용 (LLM 자연어 변환 X)
    static std::vector<schemas::SideEffectQuote>
    collect_side_effect_quotes(const std::vector<std::string>& item_codes);
};

} // namespace medibridge::services
