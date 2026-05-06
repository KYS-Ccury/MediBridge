// =====================================================
// Models — DB 테이블 ↔ C++ struct 매핑
// =====================================================
// DB ERD v2 의 테이블을 그대로 1:1 표현.
// Drogon ORM 사용 시 drogon_ctl create model 로 자동 생성 가능.
// 본 파일은 수동 정의 골격.
// =====================================================
#pragma once

#include <string>
#include <optional>
#include <chrono>

namespace medibridge::database::models {

// pill_identification — 식약처 낱알식별
struct PillIdentification {
    std::string item_code;          // PK
    std::string drug_name;
    std::string manufacturer;
    std::string shape;
    std::string color_front;
    std::string color_back;
    std::string engraving_front;
    std::string engraving_back;
    std::string pill_image_url;     // URL만 저장 (이미지 본체는 식약처 측)
    std::string last_updated;
};

// drug_overview — 식약처 e약은요 (lazy 캐싱)
struct DrugOverview {
    std::string item_code;          // PK / FK → PillIdentification
    std::string efficacy_text;
    std::string usage_text;
    std::string warning_text;
    std::string caution_text;
    std::string interaction_text;
    std::string side_effect_text;
    std::string storage_text;
    std::string cached_at;          // TTL 정책 없음
};

// ingredient_info — DUR 성분
struct IngredientInfo {
    std::string ingredient_code;    // PK
    std::string ingredient_name;
    std::string safety_info;
    std::string last_updated;
};

// pill_ingredient_mapping — 약-성분 매핑
struct PillIngredientMapping {
    int mapping_id;
    std::string item_code;
    std::string ingredient_code;
};

// dur_interaction_cache — DUR 품목 (페어형, MVP는 병용금기 우선)
struct DurInteractionCache {
    std::string dur_id;             // PK
    std::string base_item_code;
    std::string target_item_code;
    std::string dur_type;
    std::string prohibit_reason;    // 식약처 본문 그대로 (가공 X)
    std::string last_updated;
};

// users — 사용자 계정 (모듈 6)
struct User {
    std::string user_id;            // PK
    std::string email;              // UNIQUE
    std::string password_hash;      // bcrypt
    std::string user_name;
    std::string created_at;
};

// user_medication_pool — 사용자별 등록 약 풀 (모듈 1)
struct UserMedicationPool {
    int pool_id;                    // PK auto_increment
    std::string user_id;            // FK
    std::string item_code;          // FK
    std::string reg_method;         // VOICE / MANUAL / IMAGE
    bool is_active;                 // soft delete
    std::string created_at;
    std::optional<std::string> deactivated_at;
};

// medication_intake_logs — 복약 이력 (모듈 2)
struct MedicationIntakeLog {
    std::string intake_id;          // PK
    std::string user_id;
    std::string item_code;
    std::string intake_datetime;
    int quantity;
    std::optional<std::string> memo;
    std::optional<double> confidence_score;
    std::optional<std::string> dur_snapshot_json;
    std::string created_at;
};

// user_reports — 보고서 (모듈 5)
struct UserReport {
    std::string report_id;          // PK
    std::string user_id;
    std::string period_start;
    std::string period_end;
    std::string consumed_summary;   // JSON 직렬화
    std::string side_effect_quotes; // JSON 직렬화
    std::optional<std::string> report_notes;
    std::string created_at;
};

} // namespace medibridge::database::models
