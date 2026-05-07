// =====================================================
// TimeUtil — 시각 변환 유틸 (thread-safe)
// =====================================================
// std::gmtime() 은 정적 버퍼를 반환하여 멀티스레드 환경에서
// race condition 위험. Drogon은 워커 스레드 풀로 동작하므로
// 본 헬퍼로 thread-safe 변환을 보장한다.
// =====================================================
#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace medibridge::utils {

/**
 * @brief 주어진 time_t 를 ISO 8601 UTC 문자열로 변환 (thread-safe).
 *
 * 플랫폼별로 안전한 변환 함수를 사용:
 *   - Windows: gmtime_s
 *   - POSIX:   gmtime_r
 */
inline std::string to_iso8601_utc(std::time_t t)
{
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

/**
 * @brief 현재 시각을 ISO 8601 UTC 문자열로 반환 (thread-safe).
 */
inline std::string current_iso8601_utc()
{
    auto now = std::chrono::system_clock::now();
    return to_iso8601_utc(std::chrono::system_clock::to_time_t(now));
}

} // namespace medibridge::utils
