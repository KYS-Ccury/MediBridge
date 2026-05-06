// =====================================================
// PillSchema — Pill API 요청/응답 (PillApi.md)
// =====================================================
#pragma once

#include <json/json.h>
#include <string>
#include <vector>
#include <optional>

namespace medibridge::schemas {

// ----- POST /v1/pill/identify -----
struct IdentifyRequest {
    std::string image_request_id;
    std::optional<std::string> utterance_request_id;
    bool include_dur_check = true;

    static IdentifyRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

struct PillCandidate {
    std::string item_code;
    std::string drug_name;
    double confidence;
    std::vector<std::string> match_keys;     // ["engraving", "shape", "color"]
    bool in_user_pool;
    Json::Value to_json() const;
};

enum class ConfidenceTier { HIGH, MEDIUM, LOW };
std::string confidence_tier_to_string(ConfidenceTier t);
ConfidenceTier confidence_tier_from_score(double score);    // ≥0.95 HIGH, ≥0.7 MEDIUM, else LOW

struct DurDetail {
    std::string dur_type;                    // "병용금기" 등
    std::string drug_a_item_code;
    std::string drug_a_name;
    std::string drug_b_item_code;
    std::string drug_b_name;
    std::string prohibit_reason;             // 식약처 DUR 본문 그대로 인용 (가공 X)
    std::string action_message;              // "즉시 약사·의사 상담 필요" 등 정해진 템플릿
    Json::Value to_json() const;
};

struct DurCheckResult {
    std::string result;                      // "no_risk_found" / "risk_found"
    std::string checked_at;
    std::vector<DurDetail> details;
    Json::Value to_json() const;
};

struct GuidanceMessage;   // SpeechSchema.h 정의 재사용

struct IdentifyResponse {
    std::string request_id;
    std::vector<PillCandidate> candidates;
    ConfidenceTier confidence_tier;
    Json::Value guidance;                    // GuidanceMessage::to_json()
    DurCheckResult dur_check;

    Json::Value to_json() const;
};

// ----- /v1/pill/pool — CRUD -----
struct PoolItem {
    int pool_id;
    std::string item_code;
    std::string drug_name;
    std::string reg_method;                  // VOICE / MANUAL / IMAGE
    bool is_active;
    std::string created_at;
    Json::Value to_json() const;
};

struct PoolListResponse {
    std::vector<PoolItem> items;
    int total_count;
    Json::Value to_json() const;
};

struct PoolAddRequest {
    std::string item_code;
    std::string reg_method;                  // VOICE / MANUAL / IMAGE
    static PoolAddRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

} // namespace medibridge::schemas
