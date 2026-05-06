// =====================================================
// ReportHtmlRenderer — 보고서 데이터 → 인쇄 친화적 HTML
// =====================================================
#pragma once

#include <string>
#include "../../Schemas/ReportSchema.h"

namespace medibridge::services::report {

class ReportHtmlRenderer
{
public:
    /// ReportResponse → HTML 문자열 (CSS @media print 적용)
    static std::string render(const schemas::ReportResponse& report);
};

} // namespace medibridge::services::report
