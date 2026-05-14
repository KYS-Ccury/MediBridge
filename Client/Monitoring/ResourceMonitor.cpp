// =====================================================
// ResourceMonitor — Windows API 기반 CPU·메모리·디스크 측정
// =====================================================
// CPU    : GetSystemTimes 두 번 호출 차이 (idle/kernel/user)
// 메모리 : GlobalMemoryStatusEx → dwMemoryLoad (0~100)
// 디스크 : GetDiskFreeSpaceExW("C:\\") → free/total
//
// CPU 측정은 인터벌 사이 차이를 봐야 정확하므로 멤버에 이전 측정값 저장.
// =====================================================
#include "ResourceMonitor.h"

#include <QLoggingCategory>

#ifdef Q_OS_WIN
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

namespace medibridge::monitoring {

namespace {

#ifdef Q_OS_WIN
// FILETIME 을 64-bit 정수로 변환
inline quint64 filetime_to_u64(const FILETIME& ft)
{
    ULARGE_INTEGER u;
    u.LowPart  = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}
#endif

} // anonymous

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

#ifdef Q_OS_WIN
    // 첫 GetSystemTimes 호출 — 다음 측정 시 차이 계산용
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        prev_idle_   = filetime_to_u64(idle);
        prev_kernel_ = filetime_to_u64(kernel);
        prev_user_   = filetime_to_u64(user);
        has_prev_cpu_ = true;
    }
#endif

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
        << "[ResourceMonitor] CPU=" << QString::number(last_cpu_percent_, 'f', 1) << "% "
        << "MEM=" << QString::number(last_memory_percent_, 'f', 1) << "% "
        << "DISK=" << QString::number(last_disk_percent_, 'f', 1) << "%";

    emit measured(last_cpu_percent_, last_memory_percent_, last_disk_percent_);
}

double ResourceMonitor::last_cpu_percent()    const { return last_cpu_percent_; }
double ResourceMonitor::last_memory_percent() const { return last_memory_percent_; }
double ResourceMonitor::last_disk_percent()   const { return last_disk_percent_; }

// =====================================================
// Windows API 측정
// =====================================================
double ResourceMonitor::read_cpu_percent()
{
#ifdef Q_OS_WIN
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        return 0.0;
    }
    const quint64 cur_idle   = filetime_to_u64(idle);
    const quint64 cur_kernel = filetime_to_u64(kernel);
    const quint64 cur_user   = filetime_to_u64(user);

    if (!has_prev_cpu_) {
        // 첫 호출 — 차이 계산 불가
        prev_idle_   = cur_idle;
        prev_kernel_ = cur_kernel;
        prev_user_   = cur_user;
        has_prev_cpu_ = true;
        return 0.0;
    }

    const quint64 d_idle   = cur_idle   - prev_idle_;
    const quint64 d_kernel = cur_kernel - prev_kernel_;
    const quint64 d_user   = cur_user   - prev_user_;
    const quint64 d_total  = d_kernel + d_user;     // kernel 에 idle 포함됨

    prev_idle_   = cur_idle;
    prev_kernel_ = cur_kernel;
    prev_user_   = cur_user;

    if (d_total == 0) return 0.0;
    const double busy = double(d_total - d_idle);
    return (busy / double(d_total)) * 100.0;
#else
    return 0.0;
#endif
}

double ResourceMonitor::read_memory_percent()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        return double(mem.dwMemoryLoad);
    }
#endif
    return 0.0;
}

double ResourceMonitor::read_disk_percent()
{
#ifdef Q_OS_WIN
    ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
    if (GetDiskFreeSpaceExW(L"C:\\", &free_bytes, &total_bytes, &total_free_bytes)) {
        if (total_bytes.QuadPart == 0) return 0.0;
        const double used = double(total_bytes.QuadPart - free_bytes.QuadPart);
        return (used / double(total_bytes.QuadPart)) * 100.0;
    }
#endif
    return 0.0;
}

} // namespace medibridge::monitoring
