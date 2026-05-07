// =====================================================
// PillSchema — Pill API 요청/응답 (PillApi.md v0.2)
// =====================================================
// v0.2 변경 (목업 분석):
//   - PillCandidate 에 classification_name·efficacy_text·usage_text 추가
//   - PoolItem 에 user_category·classification_name 추가
//   - GuidanceMessage 에 fallback_action 추가
//   - 신규 NarrowDownRequest/Response (단계별 좁히기)
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

    // ⭐ v0.2 신규
    std::optional<std::string> classification_name;   // 식약처 약효분류명
    std::optional<std::string> efficacy_text;         // e약은요 효능 (그대로 인용)
    std::optional<std::string> usage_text;            // e약은요 사용법 (그대로 인용)

    Json::Value to_json() const;
};

enum class ConfidenceTier { HIGH, MEDIUM, LOW };
std::string confidence_tier_to_string(ConfidenceTier t);
ConfidenceTier confidence_tier_from_score(double score);

// ⭐ v0.2 신규: 식별 실패·LOW 신뢰도 시 클라 라우팅 힌트
enum class FallbackAction {
    NONE,
    NARROW_DOWN,        // 단계별 속성 질문으로 진행
    RECAPTURE,          // 다시 촬영
    CHECK_ENGRAVING,    // 각인 확인 요청
};
std::string fallback_action_to_string(FallbackAction a);

struct DurDetail {
    std::string dur_type;
    std::string drug_a_item_code;
    std::string drug_a_name;
    std::string drug_b_item_code;
    std::string drug_b_name;
    std::string prohibit_reason;             // 식약처 본문 그대로
    std::string action_message;              // 정해진 템플릿
    Json::Value to_json() const;
};

struct DurCheckResult {
    std::string result;                      // "no_risk_found" / "risk_found"
    std::string checked_at;
    std::vector<DurDetail> details;
    Json::Value to_json() const;
};

struct GuidanceMessage {
    std::string tts_text;
    std::optional<std::string> active_guide;
    std::optional<std::string> interactive_guide;
    FallbackAction fallback_action = FallbackAction::NONE;   // ⭐ v0.2 신규
    Json::Value to_json() const;
};

struct IdentifyResponse {
    std::string request_id;
    std::vector<PillCandidate> candidates;
    ConfidenceTier confidence_tier;
    GuidanceMessage guidance;
    DurCheckResult dur_check;

    Json::Value to_json() const;
};

// =====================================================
// ⭐ v0.2 신규: POST /v1/pill/identify/narrow (단계별 좁히기)
// =====================================================

/**
 * 누적 속성 — 클라가 매번 보낸 값들 (stateless).
 * std::optional 로 미입력 필드 표현.
 */
struct NarrowAttributes {
    std::optional<std::string> color;            // "흰색"/"노란색"/"빨간색"/"파란색"/"기타"
    std::optional<std::string> shape;            // "원형"/"타원형"/"장방형"/"캡슐형"/"기타"
    std::optional<std::string> has_engraving;    // "yes"/"no"/"unclear"
    std::optional<std::string> engraving_text;   // OCR 또는 사용자 입력

    static NarrowAttributes from_json(const Json::Value& json);
    Json::Value to_json() const;

    /// 모든 속성이 채워졌는지
    bool is_complete() const;
};

struct NarrowDownRequest {
    NarrowAttributes attributes;
    bool use_pool = true;
    std::optional<std::string> image_request_id;
    std::optional<std::string> utterance_request_id;

    static NarrowDownRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

/// 다음 질문을 위한 선택지 (label_kr 은 화면 표시·TTS 용)
struct NarrowOption {
    std::string value;
    std::string label;
    Json::Value to_json() const;
};

struct NarrowQuestion {
    std::string field;                       // "color"/"shape"/"has_engraving"/"engraving_text"
    std::string text_to_speak;               // TTS 출력용 한국어 질문
    std::vector<NarrowOption> options;       // engraving_text 처럼 자유 입력일 때는 비어있음
    Json::Value to_json() const;
};

struct NarrowSummaryEntry {
    std::string field;
    std::string value;
    std::string label_kr;
    Json::Value to_json() const;
};

/// 좁힘 진행 중 임시 후보 (확률 추정만)
struct NarrowCandidatePreview {
    std::string item_code;
    std::string drug_name;
    double confidence_estimate;
    Json::Value to_json() const;
};

struct NarrowDownResponse {
    int step;
    int total_steps_estimate;
    bool is_final;
    int candidates_count;

    /// is_final == false 인 경우
    std::vector<NarrowCandidatePreview> candidates_preview;
    std::optional<NarrowQuestion> next_question;

    /// is_final == true 인 경우
    std::vector<PillCandidate> final_candidates;

    /// 클라 화면 누적 표시용
    std::vector<NarrowSummaryEntry> summary_so_far;

    Json::Value to_json() const;
};

// =====================================================
// /v1/pill/pool — CRUD (v0.2 확장)
// =====================================================

struct PoolItem {
    int pool_id;
    std::string item_code;
    std::string drug_name;
    std::string reg_method;                  // VOICE / MANUAL / IMAGE
    bool is_active;
    std::string created_at;

    // ⭐ v0.2 신규
    std::optional<std::string> user_category;          // 사용자 입력 카테고리 (Slide 6 "종류")
    std::optional<std::string> classification_name;    // 식약처 약효분류 (자동 매핑)

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
    std::optional<std::string> user_category;     // ⭐ v0.2 신규 (선택, 미입력 시 자동 매핑)

    static PoolAddRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

} // namespace medibridge::schemas
