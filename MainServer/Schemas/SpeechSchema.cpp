#include "SpeechSchema.h"

namespace medibridge::schemas {

UtteranceRequest UtteranceRequest::from_json(const Json::Value& json)
{
    UtteranceRequest r;
    r.text = json.get("text", "").asString();
    if (json.isMember("stt_confidence") && !json["stt_confidence"].isNull())
        r.stt_confidence = json["stt_confidence"].asDouble();
    if (json.isMember("stt_engine") && !json["stt_engine"].isNull())
        r.stt_engine = json["stt_engine"].asString();
    if (json.isMember("context") && !json["context"].isNull())
        r.context = json["context"].asString();
    if (json.isMember("image_request_id") && !json["image_request_id"].isNull())
        r.image_request_id = json["image_request_id"].asString();
    if (json.isMember("language") && !json["language"].isNull())
        r.language = json["language"].asString();
    return r;
}

bool UtteranceRequest::is_valid(std::string& f, std::string& c) const
{
    if (text.empty())   { f = "text"; c = "MISSING_FIELD"; return false; }
    if (text.size() > 500) { f = "text"; c = "TEXT_TOO_LONG"; return false; }
    return true;
}

std::string intent_category_to_string(IntentCategory cat)
{
    switch (cat) {
        case IntentCategory::PILL_IDENTIFY:    return "PILL_IDENTIFY";
        case IntentCategory::RISK_CHECK:       return "RISK_CHECK";
        case IntentCategory::INFO_LOOKUP:      return "INFO_LOOKUP";
        case IntentCategory::REGISTER_REQUEST: return "REGISTER_REQUEST";
        case IntentCategory::HISTORY_QUERY:    return "HISTORY_QUERY";
        case IntentCategory::REPORT_REQUEST:   return "REPORT_REQUEST";
        case IntentCategory::OTHER:            return "OTHER";
    }
    return "OTHER";
}

IntentCategory intent_category_from_string(const std::string& s)
{
    if (s == "PILL_IDENTIFY")    return IntentCategory::PILL_IDENTIFY;
    if (s == "RISK_CHECK")       return IntentCategory::RISK_CHECK;
    if (s == "INFO_LOOKUP")      return IntentCategory::INFO_LOOKUP;
    if (s == "REGISTER_REQUEST") return IntentCategory::REGISTER_REQUEST;
    if (s == "HISTORY_QUERY")    return IntentCategory::HISTORY_QUERY;
    if (s == "REPORT_REQUEST")   return IntentCategory::REPORT_REQUEST;
    return IntentCategory::OTHER;
}

Json::Value IntentResult::to_json() const
{
    Json::Value v;
    v["category"]   = intent_category_to_string(category);
    v["confidence"] = confidence;
    return v;
}

Json::Value FollowUpAction::to_json() const
{
    Json::Value v;
    v["type"]   = type;
    v["params"] = params;
    return v;
}

Json::Value SpeechGuidance::to_json() const
{
    Json::Value v;
    v["tts_text"] = tts_text;
    v["active_guide"]      = active_guide.has_value()      ? Json::Value(*active_guide)      : Json::Value(Json::nullValue);
    v["interactive_guide"] = interactive_guide.has_value() ? Json::Value(*interactive_guide) : Json::Value(Json::nullValue);
    return v;
}

Json::Value UtteranceResponse::to_json() const
{
    Json::Value v;
    v["intent"]            = intent.to_json();
    v["follow_up_action"]  = follow_up_action.to_json();
    v["guidance"]          = guidance.to_json();
    v["injection_flag"]    = injection_flag;
    return v;
}

} // namespace medibridge::schemas
