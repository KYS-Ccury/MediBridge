#include "ReportHtmlRenderer.h"

namespace medibridge::services::report {

std::string ReportHtmlRenderer::render(const schemas::ReportResponse& report)
{
    // TODO (영역 B 분담):
    //   1. 인쇄 친화적 HTML 템플릿 (CSS @media print)
    //   2. 사용자 정보·기간·복약 이력 표·부작용 인용 섹션
    //   3. source_note (비단정·상담 권유) 모든 인용에 첨부
    //   4. 단정 표현 출력 검증 ("복용 가능합니다" 등 reject)
    return "<!DOCTYPE html><html><body>TODO Report HTML</body></html>";
}

} // namespace medibridge::services::report
