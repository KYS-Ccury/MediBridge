// =====================================================
// PillSchema — JSON 직렬화 구현
// =====================================================
#include "PillSchema.h"

namespace medibridge::schemas {

// ----- 헬퍼 -----
static void set_optional(Json::Value& obj, const char* key,
                         const std::optional<std::string>& v)
{
    if (v.has_value()) obj[key] = *v;
    else               obj[key] = Json::nullValue;
}

// =====================================================
// IdentifyRequest
// =====================================================
IdentifyRequest IdentifyRequest::from_json(const Json::Value& json)
{
    IdentifyRequest r;
    r.image_request_id = json.get("image_request_id", "").asString();
    if (json.isMember("utterance_request_id") && !json["utterance_request_id"].isNull()) {
        r.utterance_request_id = json["utterance_request_id"].asString();
    }
    r.include_dur_check = json.get("include_dur_check", true).asBool();
    return r;
}

bool IdentifyRequest::is_valid(std::string& error_field, std::string& error_code) const
{
    if (image_request_id.empty()) {
        error_field = "image_request_id";
        error_code  = "MISSING_FIELD";
        return false;
    }
    return true;
}

// =====================================================
// PillCandidate
// =====================================================
Json::Value PillCandidate::to_json() const
{
    Json::Value v;
    v["item_code"]    = item_code;
    v["drug_name"]    = drug_name;
    v["confidence"]   = confidence;
    v["in_user_pool"] = in_user_pool;
    Json::Value keys(Json::arrayValue);
    for (const auto& k : match_keys) keys.append(k);
    v["match_keys"] = keys;
    set_optional(v, "classification_name", classification_name);
    set_optional(v, "efficacy_text",       efficacy_text);
    set_optional(v, "usage_text",          usage_text);
    set_optional(v, "crop_image",          crop_image);
    return v;
}

// =====================================================
// ConfidenceTier
// =====================================================
std::string confidence_tier_to_string(ConfidenceTier t)
{
    switch (t) {
        case ConfidenceTier::HIGH:   return "HIGH";
        case ConfidenceTier::MEDIUM: return "MEDIUM";
        case ConfidenceTier::LOW:    return "LOW";
    }
    return "LOW";
}

ConfidenceTier confidence_tier_from_score(double score)
{
    if (score >= 0.95) return ConfidenceTier::HIGH;
    if (score >= 0.70) return ConfidenceTier::MEDIUM;
    return ConfidenceTier::LOW;
}

// =====================================================
// FallbackAction
// =====================================================
std::string fallback_action_to_string(FallbackAction a)
{
    switch (a) {
        case FallbackAction::NONE:            return "NONE";
        case FallbackAction::NARROW_DOWN:     return "NARROW_DOWN";
        case FallbackAction::RECAPTURE:       return "RECAPTURE";
        case FallbackAction::CHECK_ENGRAVING: return "CHECK_ENGRAVING";
    }
    return "NONE";
}

// =====================================================
// DurDetail / DurCheckResult
// =====================================================
Json::Value DurDetail::to_json() const
{
    Json::Value v;
    v["dur_type"]            = dur_type;
    v["drug_a_item_code"]    = drug_a_item_code;
    v["drug_a_name"]         = drug_a_name;
    v["drug_b_item_code"]    = drug_b_item_code;
    v["drug_b_name"]         = drug_b_name;
    v["prohibit_reason"]     = prohibit_reason;
    v["action_message"]      = action_message;
    return v;
}

Json::Value DurCheckResult::to_json() const
{
    Json::Value v;
    v["result"]     = result;
    v["checked_at"] = checked_at;
    Json::Value arr(Json::arrayValue);
    for (const auto& d : details) arr.append(d.to_json());
    v["details"] = arr;
    return v;
}

// =====================================================
// GuidanceMessage
// =====================================================
Json::Value GuidanceMessage::to_json() const
{
    Json::Value v;
    v["tts_text"] = tts_text;
    set_optional(v, "active_guide",      active_guide);
    set_optional(v, "interactive_guide", interactive_guide);
    v["fallback_action"] = fallback_action_to_string(fallback_action);
    return v;
}

// =====================================================
// IdentifyResponse
// =====================================================
Json::Value IdentifyResponse::to_json() const
{
    Json::Value v;
    v["request_id"]       = request_id;
    Json::Value arr(Json::arrayValue);
    for (const auto& c : candidates) arr.append(c.to_json());
    v["candidates"]       = arr;
    v["confidence_tier"]  = confidence_tier_to_string(confidence_tier);
    v["guidance"]         = guidance.to_json();
    v["dur_check"]        = dur_check.to_json();
    return v;
}

// =====================================================
// NarrowAttributes
// =====================================================
NarrowAttributes NarrowAttributes::from_json(const Json::Value& json)
{
    NarrowAttributes a;
    if (json.isMember("color") && !json["color"].isNull())
        a.color = json["color"].asString();
    if (json.isMember("shape") && !json["shape"].isNull())
        a.shape = json["shape"].asString();
    if (json.isMember("has_engraving") && !json["has_engraving"].isNull())
        a.has_engraving = json["has_engraving"].asString();
    if (json.isMember("engraving_text") && !json["engraving_text"].isNull())
        a.engraving_text = json["engraving_text"].asString();
    return a;
}

Json::Value NarrowAttributes::to_json() const
{
    Json::Value v;
    set_optional(v, "color",          color);
    set_optional(v, "shape",          shape);
    set_optional(v, "has_engraving",  has_engraving);
    set_optional(v, "engraving_text", engraving_text);
    return v;
}

bool NarrowAttributes::is_complete() const
{
    return color.has_value() && shape.has_value()
        && has_engraving.has_value()
        && (has_engraving.value() != "yes" || engraving_text.has_value());
}

// =====================================================
// NarrowDownRequest
// =====================================================
NarrowDownRequest NarrowDownRequest::from_json(const Json::Value& json)
{
    NarrowDownRequest r;
    if (json.isMember("attributes")) {
        r.attributes = NarrowAttributes::from_json(json["attributes"]);
    }
    r.use_pool = json.get("use_pool", true).asBool();
    if (json.isMember("image_request_id") && !json["image_request_id"].isNull())
        r.image_request_id = json["image_request_id"].asString();
    if (json.isMember("utterance_request_id") && !json["utterance_request_id"].isNull())
        r.utterance_request_id = json["utterance_request_id"].asString();
    return r;
}

bool NarrowDownRequest::is_valid(std::string& error_field, std::string& error_code) const
{
    // attributes 빈 객체도 허용 (첫 호출)
    static const std::vector<std::string> kColors = {"흰색","노란색","빨간색","파란색","주황","분홍","갈색","연두","기타"};
    static const std::vector<std::string> kShapes = {"원형","타원형","장방형","캡슐형","기타"};
    static const std::vector<std::string> kEngraving = {"yes","no","unclear"};

    auto in_set = [](const std::string& v, const std::vector<std::string>& s) {
        for (const auto& x : s) if (x == v) return true;
        return false;
    };
    if (attributes.color.has_value() && !in_set(*attributes.color, kColors)) {
        error_field = "color"; error_code = "INVALID_ATTRIBUTE_VALUE"; return false;
    }
    if (attributes.shape.has_value() && !in_set(*attributes.shape, kShapes)) {
        error_field = "shape"; error_code = "INVALID_ATTRIBUTE_VALUE"; return false;
    }
    if (attributes.has_engraving.has_value() && !in_set(*attributes.has_engraving, kEngraving)) {
        error_field = "has_engraving"; error_code = "INVALID_ATTRIBUTE_VALUE"; return false;
    }
    return true;
}

// =====================================================
// NarrowOption / NarrowQuestion / NarrowSummaryEntry / NarrowCandidatePreview
// =====================================================
Json::Value NarrowOption::to_json() const
{
    Json::Value v; v["value"] = value; v["label"] = label; return v;
}

Json::Value NarrowQuestion::to_json() const
{
    Json::Value v;
    v["field"]          = field;
    v["text_to_speak"]  = text_to_speak;
    Json::Value arr(Json::arrayValue);
    for (const auto& o : options) arr.append(o.to_json());
    v["options"]        = arr;
    return v;
}

Json::Value NarrowSummaryEntry::to_json() const
{
    Json::Value v;
    v["field"]    = field;
    v["value"]    = value;
    v["label_kr"] = label_kr;
    return v;
}

Json::Value NarrowCandidatePreview::to_json() const
{
    Json::Value v;
    v["item_code"]            = item_code;
    v["drug_name"]            = drug_name;
    v["confidence_estimate"]  = confidence_estimate;
    return v;
}

// =====================================================
// NarrowDownResponse
// =====================================================
Json::Value NarrowDownResponse::to_json() const
{
    Json::Value v;
    v["step"]                  = step;
    v["total_steps_estimate"]  = total_steps_estimate;
    v["is_final"]              = is_final;
    v["candidates_count"]      = candidates_count;

    if (!is_final) {
        Json::Value prev(Json::arrayValue);
        for (const auto& p : candidates_preview) prev.append(p.to_json());
        v["candidates_preview"] = prev;
        v["next_question"] = next_question.has_value()
            ? next_question->to_json() : Json::nullValue;
    } else {
        Json::Value fin(Json::arrayValue);
        for (const auto& c : final_candidates) fin.append(c.to_json());
        v["final_candidates"] = fin;
        v["next_question"]    = Json::nullValue;
    }

    Json::Value sum(Json::arrayValue);
    for (const auto& s : summary_so_far) sum.append(s.to_json());
    v["summary_so_far"] = sum;

    return v;
}

// =====================================================
// PoolItem / PoolListResponse / PoolAddRequest
// =====================================================
Json::Value PoolItem::to_json() const
{
    Json::Value v;
    v["pool_id"]    = pool_id;
    v["item_code"]  = item_code;
    v["drug_name"]  = drug_name;
    v["reg_method"] = reg_method;
    v["is_active"]  = is_active;
    v["created_at"] = created_at;
    set_optional(v, "user_category",       user_category);
    set_optional(v, "classification_name", classification_name);
    return v;
}

Json::Value PoolListResponse::to_json() const
{
    Json::Value v;
    Json::Value arr(Json::arrayValue);
    for (const auto& it : items) arr.append(it.to_json());
    v["items"]       = arr;
    v["total_count"] = total_count;
    return v;
}

PoolAddRequest PoolAddRequest::from_json(const Json::Value& json)
{
    PoolAddRequest r;
    r.item_code  = json.get("item_code", "").asString();
    r.reg_method = json.get("reg_method", "MANUAL").asString();
    if (json.isMember("user_category") && !json["user_category"].isNull())
        r.user_category = json["user_category"].asString();
    return r;
}

bool PoolAddRequest::is_valid(std::string& error_field, std::string& error_code) const
{
    if (item_code.empty()) {
        error_field = "item_code"; error_code = "MISSING_FIELD"; return false;
    }
    if (reg_method != "VOICE" && reg_method != "MANUAL" && reg_method != "IMAGE") {
        error_field = "reg_method"; error_code = "INVALID_REG_METHOD"; return false;
    }
    return true;
}

// =====================================================
// Onboarding (v0.3)
// =====================================================
std::string onboarding_state_to_string(OnboardingState s)
{
    switch (s) {
        case OnboardingState::NEED_DISAMBIGUATION: return "NEED_DISAMBIGUATION";
        case OnboardingState::RESOLVED:            return "RESOLVED";
        case OnboardingState::NOT_FOUND:           return "NOT_FOUND";
    }
    return "NOT_FOUND";
}

OnboardingPrevSelection OnboardingPrevSelection::from_json(const Json::Value& json)
{
    OnboardingPrevSelection s;
    s.field = json.get("field", "").asString();
    if (json.isMember("value") && !json["value"].isNull())
        s.value = json["value"].asString();
    if (json.isMember("item_code") && !json["item_code"].isNull())
        s.item_code = json["item_code"].asString();
    return s;
}

OnboardingNormalizeRequest OnboardingNormalizeRequest::from_json(const Json::Value& json)
{
    OnboardingNormalizeRequest r;
    if (json.isMember("utterance_request_id") && !json["utterance_request_id"].isNull())
        r.utterance_request_id = json["utterance_request_id"].asString();
    r.utterance_text = json.get("utterance_text", "").asString();
    r.round = json.get("round", 1).asInt();
    if (json.isMember("prev_choice_token") && !json["prev_choice_token"].isNull())
        r.prev_choice_token = json["prev_choice_token"].asString();
    if (json.isMember("prev_selection") && !json["prev_selection"].isNull())
        r.prev_selection = OnboardingPrevSelection::from_json(json["prev_selection"]);
    return r;
}

bool OnboardingNormalizeRequest::is_valid(std::string& error_field, std::string& error_code) const
{
    constexpr int kMaxRounds = 5;
    constexpr size_t kMaxInputTokens = 200;

    if (round < 1) {
        error_field = "round"; error_code = "INVALID_ROUND"; return false;
    }
    if (round > kMaxRounds) {
        error_field = "round"; error_code = "MAX_ROUNDS_EXCEEDED"; return false;
    }
    if (round == 1) {
        if (utterance_text.empty()) {
            error_field = "utterance_text"; error_code = "INVALID_UTTERANCE_TEXT"; return false;
        }
        if (utterance_text.size() > kMaxInputTokens) {
            error_field = "utterance_text"; error_code = "INVALID_UTTERANCE_TEXT"; return false;
        }
    } else {
        if (!prev_choice_token.has_value()) {
            error_field = "prev_choice_token"; error_code = "MISSING_PREV_CHOICE_TOKEN"; return false;
        }
    }
    return true;
}

Json::Value OnboardingCandidate::to_json() const
{
    Json::Value v;
    v["item_code"] = item_code;
    v["item_name"] = item_name;
    set_optional(v, "ingredient_name",     ingredient_name);
    set_optional(v, "classification_name", classification_name);
    set_optional(v, "manufacturer",        manufacturer);
    set_optional(v, "hint",                hint);
    return v;
}

Json::Value OnboardingOption::to_json() const
{
    Json::Value v; v["value"] = value; v["label"] = label; return v;
}

Json::Value OnboardingQuestion::to_json() const
{
    Json::Value v;
    v["field"]    = field;
    v["text"]     = text;
    v["tts_text"] = tts_text;
    Json::Value arr(Json::arrayValue);
    for (const auto& o : options) arr.append(o.to_json());
    v["options"]      = arr;
    v["choice_token"] = choice_token;
    return v;
}

Json::Value OnboardingConfirmation::to_json() const
{
    Json::Value v;
    v["text"]     = text;
    v["tts_text"] = tts_text;
    return v;
}

Json::Value OnboardingResolved::to_json() const
{
    Json::Value v;
    v["item_code"] = item_code;
    v["item_name"] = item_name;
    set_optional(v, "ingredient_name",     ingredient_name);
    set_optional(v, "classification_name", classification_name);
    set_optional(v, "manufacturer",        manufacturer);
    set_optional(v, "efficacy_text",       efficacy_text);
    set_optional(v, "usage_text",          usage_text);
    return v;
}

Json::Value OnboardingNormalizeResponse::to_json() const
{
    Json::Value v;
    v["state"]      = onboarding_state_to_string(state);
    v["round"]      = round;
    v["max_rounds"] = max_rounds;

    if (state == OnboardingState::NEED_DISAMBIGUATION) {
        v["candidates_count"] = candidates_count;
        Json::Value arr(Json::arrayValue);
        for (const auto& c : candidates) arr.append(c.to_json());
        v["candidates"] = arr;
        v["question"]   = question.has_value() ? question->to_json() : Json::nullValue;
    } else if (state == OnboardingState::RESOLVED) {
        v["resolved"]     = resolved.has_value()     ? resolved->to_json()     : Json::nullValue;
        v["confirmation"] = confirmation.has_value() ? confirmation->to_json() : Json::nullValue;
    } else {
        set_optional(v, "reason",          reason);
        set_optional(v, "tts_text",        tts_text);
        set_optional(v, "fallback_action", fallback_action);
    }
    return v;
}

} // namespace medibridge::schemas
