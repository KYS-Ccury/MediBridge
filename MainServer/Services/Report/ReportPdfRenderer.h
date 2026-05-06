// =====================================================
// ReportPdfRenderer — HTML → PDF 변환 (무거운 작업, WorkerPool 위임 권장)
// =====================================================
#pragma once

#include <string>
#include <vector>

namespace medibridge::services::report {

class ReportPdfRenderer
{
public:
    /// HTML 문자열 → PDF 바이너리
    /// 무거운 작업이므로 호출 측에서 WorkerPool::instance().submit(...) 권장
    static std::vector<unsigned char> render(const std::string& html);
};

} // namespace medibridge::services::report
