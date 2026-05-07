#include "HealthSchema.h"

namespace medibridge::schemas {

Json::Value HealthResponse::to_json() const
{
    Json::Value v;
    v["status"]         = status;
    v["service"]        = service;
    v["version"]        = version;
    v["uptime_seconds"] = static_cast<Json::Int64>(uptime_seconds);
    v["checked_at"]     = checked_at;
    Json::Value arr(Json::arrayValue);
    for (const auto& r : reasons) arr.append(r);
    v["reasons"] = arr;
    return v;
}

Json::Value SystemMetrics::to_json() const
{
    Json::Value v;
    v["cpu_percent"]      = cpu_percent;
    v["memory_used_mb"]   = memory_used_mb;
    v["memory_total_mb"]  = memory_total_mb;
    v["memory_percent"]   = memory_percent;
    v["disk_used_gb"]     = disk_used_gb;
    v["disk_total_gb"]    = disk_total_gb;
    v["disk_percent"]     = disk_percent;
    v["uptime_seconds"]   = static_cast<Json::Int64>(uptime_seconds);
    return v;
}

Json::Value ProcessMetrics::to_json() const
{
    Json::Value v;
    v["pid"]                    = pid;
    v["cpu_percent"]            = cpu_percent;
    v["memory_mb"]              = memory_mb;
    v["thread_count"]           = thread_count;
    v["open_file_descriptors"]  = open_file_descriptors;
    return v;
}

Json::Value ServiceSpecificMetrics::to_json() const
{
    Json::Value v;
    v["active_connections"]        = active_connections;
    v["request_rate_per_minute"]   = request_rate_per_minute;
    v["db_connection_pool_used"]   = db_connection_pool_used;
    v["db_connection_pool_total"]  = db_connection_pool_total;
    return v;
}

Json::Value MetricsResponse::to_json() const
{
    Json::Value v;
    v["service"]          = service;
    v["collected_at"]     = collected_at;
    v["system"]           = system.to_json();
    v["process"]          = process.to_json();
    v["service_specific"] = service_specific.to_json();
    return v;
}

} // namespace medibridge::schemas
