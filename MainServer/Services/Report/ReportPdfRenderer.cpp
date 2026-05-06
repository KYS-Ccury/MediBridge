#include "ReportPdfRenderer.h"

namespace medibridge::services::report {

std::vector<unsigned char> ReportPdfRenderer::render(const std::string& html)
{
    // TODO (영역 B 분담):
    //   - wkhtmltopdf, libharu, Chromium headless 등 선택
    //   - 한글 폰트 임베딩 필수
    //   - 무거운 작업이므로 호출 측 WorkerPool 위임
    return {};
}

} // namespace medibridge::services::report
