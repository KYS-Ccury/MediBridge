// =====================================================
// ReportSchema — Report API 요청/응답 (ReportApi.md)
// =====================================================
#pragma once

#include <json/json.h>
#include <string>
#include <vector>

namespace medibridge::schemas {

struct ReportPeriod {
    std::string from_date;
    std::string to_date;
    Json::Value to_json() const;
};

struct ConsumedByDrug {
    std::string item_code;
    std::string drug_name;
    int total_quantity;
    std::string first_intake;
    std::string last_intake;
    Json::Value to_json() const;
};

struct ConsumedSummary {
    int total_intakes;
    std::vector<ConsumedByDrug> by_drug;
    Json::Value to_json() const;
};

// 식약처 e약은요 인용 (LLM 자연어 변환 ❌)
struct SideEffectQuote {
    std::string item_code;
    std::string drug_name;
    std::string caution_text;        // e약은요 본문 그대로 인용
    std::string side_effect_text;    // e약은요 본문 그대로 인용
    std::string source_note;         // 비단정·상담 권유 톤 안내
    Json::Value to_json() const;
};

struct IntakeLogEntry {
    std::string intake_datetime;
    std::string drug_name;
    int quantity;
    std::string memo;
    Json::Value to_json() const;
};

struct ReportResponse {
    std::string report_id;
    Json::Value user;                // UserSummary 의 JSON
    ReportPeriod period;
    ConsumedSummary consumed_summary;
    std::vector<SideEffectQuote> side_effect_quotes;
    std::vector<IntakeLogEntry> intake_logs;
    std::string generated_at;

    Json::Value to_json() const;
};

} // namespace medibridge::schemas
