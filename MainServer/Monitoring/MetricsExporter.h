// =====================================================
// MetricsExporter — /metrics 응답 빌더
// =====================================================
// JSON 기본, 확장 시 Prometheus 텍스트 형식.
// =====================================================
#pragma once

#include "../Schemas/HealthSchema.h"

namespace medibridge::monitoring {

class MetricsExporter
{
public:
    static MetricsExporter& instance();

    /// 현재 메트릭 수집 → JSON 형식
    schemas::MetricsResponse collect_json();

    /// 현재 메트릭 수집 → Prometheus 텍스트 형식 (확장)
    std::string collect_prometheus();

private:
    MetricsExporter() = default;
};

} // namespace medibridge::monitoring
