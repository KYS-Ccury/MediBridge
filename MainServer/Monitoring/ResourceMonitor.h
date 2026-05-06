// =====================================================
// ResourceMonitor — 메인서버 자체 자원 측정 (Linux)
// =====================================================
// /proc/* 파싱 또는 sysinfo() 기반 CPU·메모리·디스크 측정.
// =====================================================
#pragma once

namespace medibridge::monitoring {

class ResourceMonitor
{
public:
    static ResourceMonitor& instance();

    /// 즉시 1회 측정
    void measure();

    // 최근 측정값 (MetricsExporter에서 사용)
    double last_cpu_percent() const;
    double last_memory_percent() const;
    double last_memory_used_mb() const;
    double last_memory_total_mb() const;
    double last_disk_percent() const;
    long   last_uptime_seconds() const;

private:
    ResourceMonitor() = default;

    double last_cpu_percent_      = 0.0;
    double last_memory_percent_   = 0.0;
    double last_memory_used_mb_   = 0.0;
    double last_memory_total_mb_  = 0.0;
    double last_disk_percent_     = 0.0;
    long   last_uptime_seconds_   = 0;
};

} // namespace medibridge::monitoring
