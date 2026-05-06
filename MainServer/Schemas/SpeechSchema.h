// =====================================================
// SpeechSchema — Speech API 요청/응답 (SpeechApi.md)
// =====================================================
#pragma once

#include <json/json.h>
#include <string>
#include <optional>

namespace medibridge::schemas {

// POST /v1/speech/utterance
struct UtteranceRequest {
    std::string text;
    std::optional<double> stt_confidence;
    std::optional<std::string> stt_engine;        // "galaxy_ai" 등
    std::optional<std::string> context;           // "daily_use"/"onboarding"/"confirmation"
    std::optional<std::string> image_request_id;
    std::optional<std::string> language;          // "ko-KR"

    static UtteranceRequest from_json(const Json::Value& json);
    bool is_valid(std::string& error_field, std::string& error_code) const;
};

// Stage 0.5 의도 카테고리 (FR-A5-01)
enum class IntentCategory {
    PILL_IDENTIFY,
    RISK_CHECK,
    INFO_LOOKUP,
    REGISTER_REQUEST,
    HISTORY_QUERY,
    REPORT_REQUEST,
    OTHER
};

std::string intent_category_to_string(IntentCategory cat);
IntentCategory intent_category_from_string(const std::string& s);

struct IntentResult {
    IntentCategory category;
    double confidence;
    Json::Value to_json() const;
};

struct FollowUpAction {
    std::string type;                  // 예: "ROUTE_TO_VISION"
    Json::Value params;                // 자유 형식
    Json::Value to_json() const;
};

// 주의: PillSchema 의 GuidanceMessage 와 동일 namespace 의 동일 심볼이 되지 않도록
// SpeechGuidance 로 분리. (PillSchema 의 GuidanceMessage 는 fallback_action 포함, 의미가 다름)
struct SpeechGuidance {
    std::string tts_text;
    std::optional<std::string> active_guide;
    std::optional<std::string> interactive_guide;
    Json::Value to_json() const;
};

struct UtteranceResponse {
    IntentResult intent;
    FollowUpAction follow_up_action;
    SpeechGuidance guidance;
    bool injection_flag;               // 인젝션 시도 감지 시 true → "OTHER"로 강제

    Json::Value to_json() const;
};

} // namespace medibridge::schemas
