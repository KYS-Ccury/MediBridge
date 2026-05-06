// =====================================================
// HealthSchema — Monitoring API 응답 (MonitoringApi.md)
// =====================================================
#pragma once

#include <json/json.h>
#include <string>
#include <vector>

namespace medibridge::schemas {

// ----- GET /health -----
struct HealthResponse {
    std::string status;            // "ok" / "degraded" / "down"
    std::string service;           // "main_server"
    std::string version;
    long uptime_seconds;
    std::string checked_at;        // ISO 8601
    std::vector<std::string> reasons;  // status != ok 시 사유

    Json::Value to_json() const;
};

// ----- GET /metrics -----
struct SystemMetrics {
    double cpu_percent;
    double memory_used_mb;
    double memory_total_mb;
    double memory_percent;
    double disk_used_gb;
    double disk_total_gb;
    double disk_percent;
    long uptime_seconds;
    Json::Value to_json() const;
};

struct ProcessMetrics {
    int pid;
    double cpu_percent;
    double memory_mb;
    int thread_count;
    int open_file_descriptors;
    Json::Value to_json() const;
};

struct ServiceSpecificMetrics {
    int active_connections;
    int request_rate_per_minute;
    int db_connection_pool_used;
    int db_connection_pool_total;
    Json::Value to_json() const;
};

struct MetricsResponse {
    std::string service;           // "main_server"
    std::string collected_at;
    SystemMetrics system;
    ProcessMetrics process;
    ServiceSpecificMetrics service_specific;

    Json::Value to_json() const;
};

} // namespace medibridge::schemas
