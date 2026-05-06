// =====================================================
// HealthChecker — 서비스 헬스 점검 (DB·추론서버·디스크)
// =====================================================
// /health 라우터에서 호출하여 ok/degraded/down 판정.
// =====================================================
#pragma once

#include <string>
#include <vector>
#include "../Schemas/HealthSchema.h"

namespace medibridge::monitoring {

class HealthChecker
{
public:
    static HealthChecker& instance();

    /// 현재 상태 점검 → HealthResponse 반환
    schemas::HealthResponse check();

private:
    HealthChecker() = default;

    /// 의존 서비스 점검 — 실패 사유를 reasons 에 누적
    bool check_db(std::vector<std::string>& reasons);
    bool check_inference_server(std::vector<std::string>& reasons);
    bool check_disk(std::vector<std::string>& reasons);
};

} // namespace medibridge::monitoring
