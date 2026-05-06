#include "ResourceMonitor.h"

namespace medibridge::monitoring {

ResourceMonitor& ResourceMonitor::instance()
{
    static ResourceMonitor instance;
    return instance;
}

void ResourceMonitor::measure()
{
    // TODO (영역 B 분담):
    //   1. /proc/stat 파싱 → cpu_percent (앞 수치 합 차이 / 총 차이)
    //   2. sysinfo() 호출 → totalram, freeram → memory_*
    //   3. statvfs("/") → 디스크 사용률
    //   4. /proc/uptime → uptime_seconds
    //
    // 참고: MonitoringApi.md SystemMetrics
    last_cpu_percent_     = 0.0;
    last_memory_percent_  = 0.0;
    last_memory_used_mb_  = 0.0;
    last_memory_total_mb_ = 0.0;
    last_disk_percent_    = 0.0;
    last_uptime_seconds_  = 0;
}

double ResourceMonitor::last_cpu_percent()      const { return last_cpu_percent_; }
double ResourceMonitor::last_memory_percent()   const { return last_memory_percent_; }
double ResourceMonitor::last_memory_used_mb()   const { return last_memory_used_mb_; }
double ResourceMonitor::last_memory_total_mb()  const { return last_memory_total_mb_; }
double ResourceMonitor::last_disk_percent()     const { return last_disk_percent_; }
long   ResourceMonitor::last_uptime_seconds()   const { return last_uptime_seconds_; }

} // namespace medibridge::monitoring
