// =====================================================
// HistorySchema — History API 요청/응답 (HistoryApi.md)
// =====================================================
#pragma once

#include <json/json.h>
#include <string>
#include <vector>
#include <optional>

namespace medibridge::schemas {

// ----- POST /v1/history/record -----
struct DurSnapshot {
    std::string checked_at;
    std::string result;            // "no_risk_found" / "risk_found"
    Json::Value details;           // 원문 그대로 보존 (가공 X)

    static DurSnapshot from_json(const Json::Value& json);
    Json::Value to_json() const;
};

struct HistoryRecordRequest {
    std::string item_code;
    std::string intake_datetime;   // 미입력 시 서버 현재 시각
    int quantity;
    std::optional<std::string> memo;
    std::optional<double> confidence_score;
    std::optional<DurSnapshot> dur_snapshot;

    static HistoryRecordRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct HistoryRecordResponse {
    std::string intake_id;
    std::string user_id;
    std::string item_code;
    std::string intake_datetime;
    int quantity;
    std::string memo;
    std::string created_at;

    Json::Value to_json() const;
};

// ----- GET /v1/history/list -----
struct HistoryListQuery {
    std::optional<std::string> from_date;
    std::optional<std::string> to_date;
    std::optional<std::string> item_code;
    int page = 1;
    int page_size = 20;
    std::string sort_order = "desc";
};

struct HistoryItem {
    std::string intake_id;
    std::string item_code;
    std::string drug_name;
    std::string intake_datetime;
    int quantity;
    std::string memo;
    std::string time_slot;         // "아침" / "점심" / "저녁" / "취침"

    Json::Value to_json() const;
};

struct HistoryListResponse {
    std::vector<HistoryItem> items;
    int page;
    int page_size;
    int total_count;
    int total_pages;

    Json::Value to_json() const;
};

} // namespace medibridge::schemas
