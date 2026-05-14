// =====================================================
// Speech Router — TestMode: 키워드 매칭 의도 분류 더미
// =====================================================
#include "Speech.h"
#include "../Schemas/SpeechSchema.h"
#include "../Config.h"
#include "../Services/Auth/JwtIssuer.h"

#include <drogon/HttpResponse.h>
#include <trantor/utils/Logger.h>

using medibridge::Config;
namespace AuthSvc = medibridge::services::auth;

namespace medibridge::routers {

static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message,
                                              const std::string& field = "")
{
    Json::Value body, err;
    err["code"] = err_code; err["message"] = message;
    if (!field.empty()) {
        Json::Value details; details["field"] = field;
        err["details"] = details;
    }
    body["error"] = err;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

/// 인젝션 1차 필터 — 보수적 패턴 매칭
static bool looks_like_injection(const std::string& text)
{
    static const std::vector<std::string> kBad = {
        "이전 지시", "이전지시", "instructions",
        "당신은 의사", "너는 의사", "act as a doctor",
        "system prompt", "system role", "ignore previous",
    };
    for (const auto& p : kBad) {
        if (text.find(p) != std::string::npos) return true;
    }
    return false;
}

/// TestMode 의도 분류 — 단순 키워드 룰
static schemas::IntentCategory classify(const std::string& text)
{
    if (text.find("등록") != std::string::npos
     || text.find("추가") != std::string::npos)            return schemas::IntentCategory::REGISTER_REQUEST;
    if (text.find("이력") != std::string::npos
     || text.find("먹은") != std::string::npos)            return schemas::IntentCategory::HISTORY_QUERY;
    if (text.find("보고서") != std::string::npos
     || text.find("리포트") != std::string::npos)          return schemas::IntentCategory::REPORT_REQUEST;
    if (text.find("위험") != std::string::npos
     || text.find("같이 먹어도") != std::string::npos)     return schemas::IntentCategory::RISK_CHECK;
    if (text.find("뭐야") != std::string::npos
     || text.find("효능") != std::string::npos
     || text.find("부작용") != std::string::npos)          return schemas::IntentCategory::INFO_LOOKUP;
    if (text.find("식별") != std::string::npos
     || text.find("이 약") != std::string::npos)           return schemas::IntentCategory::PILL_IDENTIFY;
    return schemas::IntentCategory::OTHER;
}

void Speech::handle_utterance(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto h = req->getHeader("Authorization");
    if (!AuthSvc::JwtIssuer::extract_user_id_from_header(h).has_value()) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::UtteranceRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "잘못된 발화 텍스트", err_field));
        return;
    }

    schemas::UtteranceResponse resp;
    resp.injection_flag = looks_like_injection(reqobj.text);
    resp.intent.category   = resp.injection_flag
        ? schemas::IntentCategory::OTHER
        : classify(reqobj.text);
    resp.intent.confidence = resp.injection_flag ? 0.30 : 0.85;

    // ⭐ TestMode 한정 dev 로깅 — 본문 일부 + 분류 결과
    //   PII 보호 정책상 production 에서는 절대 본문 안 찍음.
    //   TestMode 에서만 통합 검증을 위해 앞 80자 + 길이 + 분류 결과 출력.
    if (Config::instance().test_mode()) {
        std::string preview = reqobj.text.substr(0, 80);
        if (reqobj.text.size() > 80) preview += "...";
        const char* cat_str = "OTHER";
        switch (resp.intent.category) {
            case schemas::IntentCategory::PILL_IDENTIFY:    cat_str = "PILL_IDENTIFY"; break;
            case schemas::IntentCategory::RISK_CHECK:       cat_str = "RISK_CHECK"; break;
            case schemas::IntentCategory::INFO_LOOKUP:      cat_str = "INFO_LOOKUP"; break;
            case schemas::IntentCategory::REGISTER_REQUEST: cat_str = "REGISTER_REQUEST"; break;
            case schemas::IntentCategory::HISTORY_QUERY:    cat_str = "HISTORY_QUERY"; break;
            case schemas::IntentCategory::REPORT_REQUEST:   cat_str = "REPORT_REQUEST"; break;
            default: break;
        }
        LOG_INFO << "[Speech][DEV] len=" << reqobj.text.size()
                 << " cat=" << cat_str
                 << " inj=" << (resp.injection_flag ? "Y" : "N")
                 << " text=\"" << preview << "\"";
    }

    resp.follow_up_action.type   = "NONE";
    resp.follow_up_action.params = Json::Value(Json::objectValue);
    if (!resp.injection_flag) {
        switch (resp.intent.category) {
            case schemas::IntentCategory::PILL_IDENTIFY:
                resp.follow_up_action.type = "ROUTE_TO_VISION"; break;
            case schemas::IntentCategory::REGISTER_REQUEST:
                resp.follow_up_action.type = "ROUTE_TO_ONBOARDING"; break;
            case schemas::IntentCategory::HISTORY_QUERY:
                resp.follow_up_action.type = "OPEN_HISTORY_VIEW"; break;
            case schemas::IntentCategory::REPORT_REQUEST:
                resp.follow_up_action.type = "OPEN_REPORT_VIEW"; break;
            default: break;
        }
    }

    resp.guidance.tts_text = resp.injection_flag
        ? "죄송해요, 의료 안내는 약사·의사 상담을 권유드려요."
        : "확인했어요. 진행할게요.";

    callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
}

} // namespace medibridge::routers
