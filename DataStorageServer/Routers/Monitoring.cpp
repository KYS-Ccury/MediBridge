// =====================================================
// Monitoring — /health
// =====================================================
// storage_root 디렉터리 존재 + free disk 측정.
// =====================================================
#include "Monitoring.h"
#include "../Config.h"

#include <drogon/HttpResponse.h>
#include <filesystem>
#include <sys/statvfs.h>

namespace datastorage::routers {

namespace fs = std::filesystem;

void Monitoring::handle_health(const drogon::HttpRequestPtr& /*req*/,
                               std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    Json::Value body;
    body["service"] = "datastorage";
    body["status"]  = "ok";

    const auto root = Config::instance().storage_root();
    body["storage_root"] = root;

    std::error_code ec;
    body["storage_root_exists"] = fs::exists(root, ec);

    // 디스크 여유
    struct statvfs vfs{};
    if (statvfs(root.c_str(), &vfs) == 0) {
        const auto free_bytes = static_cast<long long>(vfs.f_bavail) * vfs.f_frsize;
        const auto total_bytes = static_cast<long long>(vfs.f_blocks) * vfs.f_frsize;
        body["disk_free_bytes"]  = static_cast<Json::Int64>(free_bytes);
        body["disk_total_bytes"] = static_cast<Json::Int64>(total_bytes);
    } else {
        body["status"] = "degraded";
        body["reason"] = "statvfs_failed";
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    callback(resp);
}

} // namespace datastorage::routers
