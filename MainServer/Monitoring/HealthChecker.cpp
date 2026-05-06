#include "HealthChecker.h"
#include "../Database/Connection.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace medibridge::monitoring {

HealthChecker& HealthChecker::instance()
{
    static HealthChecker instance;
    return instance;
}

schemas::HealthResponse HealthChecker::check()
{
    schemas::HealthResponse resp;
    resp.service = "main_server";
    resp.version = "0.1.0";
    resp.uptime_seconds = 0;   // TODO

    // ISO 8601 현재 시각
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    resp.checked_at = ss.str();

    // TODO (영역 B 분담):
    //   1. check_db() / check_inference_server() / check_disk() 순차 호출
    //   2. 모두 OK 면 status = "ok"
    //   3. 일부 실패면 "degraded" + reasons 채움
    //   4. DB 실패면 "down"
    bool db_ok        = check_db(resp.reasons);
    bool inference_ok = check_inference_server(resp.reasons);
    bool disk_ok      = check_disk(resp.reasons);

    if (!db_ok) {
        resp.status = "down";
    } else if (!inference_ok || !disk_ok) {
        resp.status = "degraded";
    } else {
        resp.status = "ok";
    }

    return resp;
}

bool HealthChecker::check_db(std::vector<std::string>& reasons)
{
    // TODO: Database::Connection::instance().ping()
    if (!medibridge::database::Connection::instance().ping()) {
        reasons.push_back("db_connection_failed");
        return false;
    }
    return true;
}

bool HealthChecker::check_inference_server(std::vector<std::string>& reasons)
{
    // TODO: drogon::HttpClient 로 InferenceServer /health 3초 타임아웃 호출
    reasons.push_back("inference_server_check_not_implemented");
    return false;
}

bool HealthChecker::check_disk(std::vector<std::string>& reasons)
{
    // TODO: statvfs("/") → free space < 1GB 면 false + "low_disk_space"
    return true;
}

} // namespace medibridge::monitoring
