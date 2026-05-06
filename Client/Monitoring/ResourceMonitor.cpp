// =====================================================
// ResourceMonitor 구현 — 골격 (Windows API 부분 TODO)
// =====================================================
#include "ResourceMonitor.h"

#include <QLoggingCategory>

namespace medibridge::monitoring {

ResourceMonitor::ResourceMonitor(int interval_ms, QObject* parent)
    : QObject(parent)
    , interval_ms_(interval_ms)
    , last_cpu_percent_(0.0)
    , last_memory_percent_(0.0)
    , last_disk_percent_(0.0)
{
    timer_.setInterval(interval_ms_);
    connect(&timer_, &QTimer::timeout, this, &ResourceMonitor::measure_now);
}

void ResourceMonitor::start()
{
    timer_.start();
    qInfo() << "[ResourceMonitor] 시작됨 — 주기:" << interval_ms_ << "ms";
    measure_now();   // 첫 측정 즉시
}

void ResourceMonitor::stop()
{
    timer_.stop();
    qInfo() << "[ResourceMonitor] 정지됨";
}

void ResourceMonitor::measure_now()
{
    last_cpu_percent_    = read_cpu_percent();
    last_memory_percent_ = read_memory_percent();
    last_disk_percent_   = read_disk_percent();

    qInfo().nospace()
        << "[ResourceMonitor] CPU=" << last_cpu_percent_ << "% "
        << "MEM=" << last_memory_percent_ << "% "
        << "DISK=" << last_disk_percent_ << "%";

    emit measured(last_cpu_percent_, last_memory_percent_, last_disk_percent_);
}

double ResourceMonitor::last_cpu_percent()    const { return last_cpu_percent_; }
double ResourceMonitor::last_memory_percent() const { return last_memory_percent_; }
double ResourceMonitor::last_disk_percent()   const { return last_disk_percent_; }

// =====================================================
// 측정 함수 — Windows API (TODO)
// =====================================================
double ResourceMonitor::read_cpu_percent()
{
    // TODO (영역 C 분담):
    //   - Windows: PDH 라이브러리 (Pdh.h) 의 PdhCollectQueryData
    //     또는 GetSystemTimes(idle, kernel, user) 두 번 호출 후 차이 계산
    //   - 라이브러리 추가: target_link_libraries(... Pdh)
    return 0.0;
}

double ResourceMonitor::read_memory_percent()
{
    // TODO (영역 C 분담):
    //   - GlobalMemoryStatusEx(&memInfo) 호출
    //   - memInfo.dwMemoryLoad 가 0~100 percent
    return 0.0;
}

double ResourceMonitor::read_disk_percent()
{
    // TODO (영역 C 분담):
    //   - GetDiskFreeSpaceEx(L"C:\\", ...) 로 used/total 계산
    return 0.0;
}

} // namespace medibridge::monitoring
