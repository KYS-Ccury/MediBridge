// =====================================================
// Pill Router — TestMode 동작 코드
// =====================================================
// TestMode 시: 추론 서버 우회. 본 라우터가 DB seed 에서 직접 더미 응답 생성.
// Production 시: InferenceClient 호출 (TODO 영역).
// =====================================================
#include "Pill.h"
#include "../Schemas/PillSchema.h"
#include "../Database/Connection.h"
#include "../Config.h"
#include "../Services/TestMode/MockAuth.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <string>
#include <sstream>
#include <chrono>

using medibridge::database::Connection;
using medibridge::Config;
namespace TestMode = medibridge::testmode;
namespace orm = drogon::orm;

namespace medibridge::routers {

// =====================================================
// 공통 헬퍼
// =====================================================
static drogon::HttpResponsePtr json_response(drogon::HttpStatusCode code,
                                             const Json::Value& body)
{
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message,
                                              const std::string& field = "")
{
    Json::Value body, err;
    err["code"]    = err_code;
    err["message"] = message;
    if (!field.empty()) {
        Json::Value details;
        details["field"] = field;
        err["details"] = details;
    }
    body["error"] = err;
    return json_response(code, body);
}

/// Authorization 헤더에서 user_id 추출 (TestMode: mock JWT, Production: 실 JWT — TODO)
static std::optional<std::string> auth_user_id(const drogon::HttpRequestPtr& req)
{
    auto h = req->getHeader("Authorization");
    if (h.empty()) return std::nullopt;

    if (Config::instance().test_mode()) {
        return TestMode::extract_user_id_from_bearer(h);
    }
    // TODO: Services/Auth/JwtIssuer 로 검증 (영역 B 분담)
    return TestMode::extract_user_id_from_bearer(h);
}

/// user_id → anonymous_id (활성 매핑)
/// 없으면 std::nullopt
static std::optional<std::string> resolve_anonymous_id(const std::string& user_id)
{
    auto db = Connection::instance().client();
    if (!db) return std::nullopt;
    try {
        auto r = db->execSqlSync(
            "SELECT anonymous_id FROM pseudonym_map "
            "WHERE user_id = ? AND severed_at IS NULL LIMIT 1",
            user_id);
        if (r.empty()) return std::nullopt;
        return r[0]["anonymous_id"].as<std::string>();
    } catch (const orm::DrogonDbException& e) {
        return std::nullopt;
    }
}

// 짧은 random hex 토큰 (choice_token 등에 사용)
static std::string random_token(size_t n_bytes = 8)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << std::hex;
    for (size_t i = 0; i < n_bytes; ++i)
        ss << (gen() & 0xFF);
    return ss.str();
}

// =====================================================
// POST /v1/pill/identify  — TestMode: seed 에서 후보 추출
// =====================================================
void Pill::handle_identify(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 토큰이 유효하지 않습니다."));
        return;
    }
    const auto anon_opt = resolve_anonymous_id(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별에 실패했습니다."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "요청 본문이 JSON 이 아닙니다."));
        return;
    }
    auto reqobj = schemas::IdentifyRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드 누락 또는 형식 오류", err_field));
        return;
    }

    if (!Config::instance().test_mode()) {
        // TODO (운영 모드): InferenceClient::vision().detect_and_analyze() 호출
        callback(error_response(drogon::k501NotImplemented, "NOT_IMPLEMENTED", "운영 모드 추론은 아직 미구현입니다."));
        return;
    }

    // ----- TestMode: seed 활용 -----
    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 연결이 준비되지 않았습니다."));
        return;
    }

    try {
        // 사용자 약 풀과 결합 — 풀에 있으면 in_user_pool=true 로 표기, 그 외 seed pill 도 일부 포함
        auto rows = db->execSqlSync(
            "SELECT p.item_code, p.drug_name, p.classification_name, "
            "       d.efficacy_text, d.usage_text, "
            "       (SELECT 1 FROM user_medication_pool ump "
            "        WHERE ump.anonymous_id = ? AND ump.item_code = p.item_code "
            "          AND ump.is_active = TRUE LIMIT 1) AS in_pool "
            "FROM pill_identification p "
            "LEFT JOIN drug_overview d ON d.item_code = p.item_code "
            "ORDER BY in_pool DESC, p.item_code "
            "LIMIT 3",
            *anon_opt);

        schemas::IdentifyResponse resp;
        resp.request_id     = reqobj.image_request_id;
        resp.confidence_tier = schemas::ConfidenceTier::HIGH;

        double conf = 0.95;
        for (auto row : rows) {
            schemas::PillCandidate c;
            c.item_code  = row["item_code"].as<std::string>();
            c.drug_name  = row["drug_name"].as<std::string>();
            c.confidence = conf;
            c.match_keys = {"engraving", "shape", "color"};
            c.in_user_pool = !row["in_pool"].isNull() && row["in_pool"].as<int>() == 1;
            if (!row["classification_name"].isNull())
                c.classification_name = row["classification_name"].as<std::string>();
            if (!row["efficacy_text"].isNull())
                c.efficacy_text = row["efficacy_text"].as<std::string>();
            if (!row["usage_text"].isNull())
                c.usage_text = row["usage_text"].as<std::string>();
            resp.candidates.push_back(std::move(c));
            conf -= 0.10;
        }

        // GuidanceMessage
        resp.guidance.tts_text = resp.candidates.empty()
            ? "약을 식별하지 못했어요. 다시 촬영해 주세요."
            : "식별된 약 정보를 화면에 표시했어요. 약사·의사 상담을 권유드려요.";
        resp.guidance.fallback_action = resp.candidates.empty()
            ? schemas::FallbackAction::RECAPTURE : schemas::FallbackAction::NONE;

        // DUR check (TestMode 도 실 DB 조회)
        resp.dur_check.result     = "no_risk_found";
        resp.dur_check.checked_at = utils::current_iso8601_utc();
        if (reqobj.include_dur_check && !resp.candidates.empty()) {
            // 후보 첫 약과 사용자 약 풀 사이 DUR 페어 매칭
            const auto& base_code = resp.candidates.front().item_code;
            auto durs = db->execSqlSync(
                "SELECT dur.dur_id, dur.dur_type, dur.prohibit_reason, "
                "       a.drug_name AS a_name, b.drug_name AS b_name, "
                "       dur.base_item_code, dur.target_item_code "
                "FROM dur_interaction_cache dur "
                "JOIN pill_identification a ON a.item_code = dur.base_item_code "
                "JOIN pill_identification b ON b.item_code = dur.target_item_code "
                "WHERE dur.base_item_code = ? "
                "  AND dur.target_item_code IN ("
                "    SELECT item_code FROM user_medication_pool "
                "    WHERE anonymous_id = ? AND is_active = TRUE)",
                base_code, *anon_opt);

            for (auto row : durs) {
                schemas::DurDetail d;
                d.dur_type            = row["dur_type"].as<std::string>();
                d.drug_a_item_code    = row["base_item_code"].as<std::string>();
                d.drug_a_name         = row["a_name"].as<std::string>();
                d.drug_b_item_code    = row["target_item_code"].as<std::string>();
                d.drug_b_name         = row["b_name"].as<std::string>();
                d.prohibit_reason     = row["prohibit_reason"].as<std::string>();
                d.action_message      = "식약처 안내에 따라 약사·의사와 상담을 권유드립니다.";
                resp.dur_check.details.push_back(std::move(d));
            }
            if (!resp.dur_check.details.empty()) resp.dur_check.result = "risk_found";
        }

        callback(json_response(drogon::k200OK, resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

// =====================================================
// POST /v1/pill/identify/narrow  — TestMode: 속성 기반 후보 좁히기
// =====================================================
void Pill::handle_identify_narrow(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    const auto anon_opt = resolve_anonymous_id(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::NarrowDownRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "잘못된 속성 값입니다.", err_field));
        return;
    }

    if (!Config::instance().test_mode()) {
        callback(error_response(drogon::k501NotImplemented, "NOT_IMPLEMENTED", "운영 모드 narrow 미구현."));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }

    // 동적 WHERE 절 — 파라미터 바인딩 유지 (인젝션 방어)
    std::string sql = "SELECT item_code, drug_name, shape, color_front, "
                      "       engraving_front, classification_name "
                      "FROM pill_identification WHERE 1=1 ";
    std::vector<std::string> params;

    if (reqobj.attributes.color.has_value()) {
        sql += " AND color_front = ?";
        params.push_back(*reqobj.attributes.color);
    }
    if (reqobj.attributes.shape.has_value()) {
        sql += " AND shape = ?";
        params.push_back(*reqobj.attributes.shape);
    }
    if (reqobj.attributes.has_engraving.has_value() && *reqobj.attributes.has_engraving == "no") {
        sql += " AND (engraving_front IS NULL OR engraving_front = '')";
    } else if (reqobj.attributes.has_engraving.has_value() && *reqobj.attributes.has_engraving == "yes") {
        sql += " AND engraving_front IS NOT NULL AND engraving_front <> ''";
        if (reqobj.attributes.engraving_text.has_value()) {
            sql += " AND engraving_front LIKE ?";
            params.push_back("%" + *reqobj.attributes.engraving_text + "%");
        }
    }
    sql += " LIMIT 50";

    try {
        // execSqlSync 는 가변 인자 — vector 를 풀어서 호출
        orm::Result rows = [&]() {
            switch (params.size()) {
                case 0: return db->execSqlSync(sql);
                case 1: return db->execSqlSync(sql, params[0]);
                case 2: return db->execSqlSync(sql, params[0], params[1]);
                case 3: return db->execSqlSync(sql, params[0], params[1], params[2]);
                default: return db->execSqlSync(sql, params[0], params[1], params[2], params[3]);
            }
        }();

        schemas::NarrowDownResponse resp;
        resp.candidates_count = static_cast<int>(rows.size());

        // 누적 step 계산 (채워진 속성 수)
        int filled = 0;
        if (reqobj.attributes.color.has_value())          ++filled;
        if (reqobj.attributes.shape.has_value())          ++filled;
        if (reqobj.attributes.has_engraving.has_value())  ++filled;
        if (reqobj.attributes.engraving_text.has_value()) ++filled;
        resp.step                 = filled + 1;
        resp.total_steps_estimate = 4;

        // summary_so_far
        auto add_summary = [&](const std::string& field, const std::string& v, const std::string& q_kr) {
            schemas::NarrowSummaryEntry e;
            e.field = field; e.value = v; e.label_kr = q_kr + " → " + v;
            resp.summary_so_far.push_back(std::move(e));
        };
        if (reqobj.attributes.color.has_value())          add_summary("color",          *reqobj.attributes.color,          "알약의 색상은?");
        if (reqobj.attributes.shape.has_value())          add_summary("shape",          *reqobj.attributes.shape,          "알약의 모양은?");
        if (reqobj.attributes.has_engraving.has_value())  add_summary("has_engraving",  *reqobj.attributes.has_engraving,  "각인이 있나요?");
        if (reqobj.attributes.engraving_text.has_value()) add_summary("engraving_text", *reqobj.attributes.engraving_text, "각인 텍스트");

        // 종료 조건 결정
        if (resp.candidates_count <= 3 || reqobj.attributes.is_complete()) {
            resp.is_final = true;
            for (auto row : rows) {
                schemas::PillCandidate c;
                c.item_code  = row["item_code"].as<std::string>();
                c.drug_name  = row["drug_name"].as<std::string>();
                c.confidence = 0.85;
                c.match_keys = {"shape", "color"};
                c.in_user_pool = false;
                if (!row["classification_name"].isNull())
                    c.classification_name = row["classification_name"].as<std::string>();
                resp.final_candidates.push_back(std::move(c));
            }
        } else {
            resp.is_final = false;
            for (auto row : rows) {
                schemas::NarrowCandidatePreview p;
                p.item_code            = row["item_code"].as<std::string>();
                p.drug_name            = row["drug_name"].as<std::string>();
                p.confidence_estimate  = 1.0 / resp.candidates_count;
                resp.candidates_preview.push_back(std::move(p));
                if (resp.candidates_preview.size() >= 5) break;
            }

            // 다음 질문 결정 — 우선순위: engraving_text > has_engraving > shape > color
            schemas::NarrowQuestion q;
            if (reqobj.attributes.has_engraving.has_value()
                && *reqobj.attributes.has_engraving == "yes"
                && !reqobj.attributes.engraving_text.has_value())
            {
                q.field = "engraving_text";
                q.text_to_speak = "약에 적힌 글자나 숫자를 알려주세요.";
            } else if (!reqobj.attributes.has_engraving.has_value()) {
                q.field = "has_engraving";
                q.text_to_speak = "약에 글자나 숫자가 새겨져 있나요?";
                q.options = {{"yes","있음"},{"no","없음"},{"unclear","잘 모르겠음"}};
            } else if (!reqobj.attributes.shape.has_value()) {
                q.field = "shape";
                q.text_to_speak = "알약의 모양은 어떤가요?";
                q.options = {{"원형","원형"},{"타원형","타원형"},{"장방형","장방형"},{"캡슐형","캡슐형"},{"기타","기타"}};
            } else {
                q.field = "color";
                q.text_to_speak = "알약의 색상은 어떤가요?";
                q.options = {{"흰색","흰색"},{"노란색","노란색"},{"빨간색","빨간색"},{"파란색","파란색"},{"기타","기타"}};
            }
            resp.next_question = q;
        }

        callback(json_response(drogon::k200OK, resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

// =====================================================
// POST /v1/pill/onboarding/normalize  — TestMode: drug_name LIKE 매칭
// =====================================================
void Pill::handle_onboarding_normalize(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::OnboardingNormalizeRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        const auto status = (err_code == "MAX_ROUNDS_EXCEEDED")
            ? drogon::k422UnprocessableEntity : drogon::k400BadRequest;
        callback(error_response(status, err_code, "Onboarding 요청 검증 실패", err_field));
        return;
    }

    if (!Config::instance().test_mode()) {
        callback(error_response(drogon::k501NotImplemented, "NOT_IMPLEMENTED", "운영 모드 onboarding 미구현."));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }

    // round=2+ 이면 prev_selection 으로 단일 후보 확정 흐름
    if (reqobj.round > 1 && reqobj.prev_selection.has_value()
        && reqobj.prev_selection->field == "candidate_pick"
        && reqobj.prev_selection->item_code.has_value())
    {
        try {
            auto rows = db->execSqlSync(
                "SELECT p.item_code, p.drug_name, p.classification_name, p.manufacturer, "
                "       d.efficacy_text, d.usage_text "
                "FROM pill_identification p "
                "LEFT JOIN drug_overview d ON d.item_code = p.item_code "
                "WHERE p.item_code = ? LIMIT 1",
                *reqobj.prev_selection->item_code);

            if (rows.empty()) {
                schemas::OnboardingNormalizeResponse resp;
                resp.state = schemas::OnboardingState::NOT_FOUND;
                resp.round = reqobj.round;
                resp.reason = "선택하신 약을 찾을 수 없어요.";
                resp.tts_text = "선택한 약을 찾지 못했어요. 다시 시도해 주세요.";
                resp.fallback_action = "RECAPTURE_OR_MANUAL";
                callback(json_response(drogon::k200OK, resp.to_json()));
                return;
            }

            auto row = rows[0];
            schemas::OnboardingResolved rv;
            rv.item_code = row["item_code"].as<std::string>();
            rv.item_name = row["drug_name"].as<std::string>();
            if (!row["classification_name"].isNull())
                rv.classification_name = row["classification_name"].as<std::string>();
            if (!row["manufacturer"].isNull())
                rv.manufacturer = row["manufacturer"].as<std::string>();
            if (!row["efficacy_text"].isNull())
                rv.efficacy_text = row["efficacy_text"].as<std::string>();
            if (!row["usage_text"].isNull())
                rv.usage_text = row["usage_text"].as<std::string>();

            schemas::OnboardingConfirmation cf;
            cf.text     = rv.item_name + " 으로 등록할게요. 맞으신가요?";
            cf.tts_text = rv.item_name + " 으로 등록할게요. 맞으세요?";

            schemas::OnboardingNormalizeResponse resp;
            resp.state = schemas::OnboardingState::RESOLVED;
            resp.round = reqobj.round;
            resp.resolved = std::move(rv);
            resp.confirmation = std::move(cf);

            callback(json_response(drogon::k200OK, resp.to_json()));
            return;
        } catch (const orm::DrogonDbException& e) {
            callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
            return;
        }
    }

    // round=1: 발화 텍스트로 약명 LIKE 매칭
    try {
        const std::string like = "%" + reqobj.utterance_text + "%";
        auto rows = db->execSqlSync(
            "SELECT p.item_code, p.drug_name, p.classification_name, p.manufacturer, "
            "       d.efficacy_text, d.usage_text, "
            "       i.ingredient_name "
            "FROM pill_identification p "
            "LEFT JOIN drug_overview d ON d.item_code = p.item_code "
            "LEFT JOIN pill_ingredient_mapping pm ON pm.item_code = p.item_code "
            "LEFT JOIN ingredient_info i ON i.ingredient_code = pm.ingredient_code "
            "WHERE p.drug_name LIKE ? "
            "ORDER BY p.item_code "
            "LIMIT 5",
            like);

        if (rows.empty()) {
            schemas::OnboardingNormalizeResponse resp;
            resp.state = schemas::OnboardingState::NOT_FOUND;
            resp.round = reqobj.round;
            resp.reason = "발화에서 약명 후보를 찾지 못했어요.";
            resp.tts_text = "약 이름을 듣지 못했어요. 다시 말씀해 주시거나 직접 입력해 주세요.";
            resp.fallback_action = "RECAPTURE_OR_MANUAL";
            callback(json_response(drogon::k200OK, resp.to_json()));
            return;
        }

        if (rows.size() == 1) {
            auto row = rows[0];
            schemas::OnboardingResolved rv;
            rv.item_code = row["item_code"].as<std::string>();
            rv.item_name = row["drug_name"].as<std::string>();
            if (!row["ingredient_name"].isNull())
                rv.ingredient_name = row["ingredient_name"].as<std::string>();
            if (!row["classification_name"].isNull())
                rv.classification_name = row["classification_name"].as<std::string>();
            if (!row["manufacturer"].isNull())
                rv.manufacturer = row["manufacturer"].as<std::string>();
            if (!row["efficacy_text"].isNull())
                rv.efficacy_text = row["efficacy_text"].as<std::string>();
            if (!row["usage_text"].isNull())
                rv.usage_text = row["usage_text"].as<std::string>();

            schemas::OnboardingConfirmation cf;
            cf.text     = rv.item_name + " 으로 등록할게요. 맞으신가요?";
            cf.tts_text = rv.item_name + " 으로 등록할게요. 맞으세요?";

            schemas::OnboardingNormalizeResponse resp;
            resp.state = schemas::OnboardingState::RESOLVED;
            resp.round = reqobj.round;
            resp.resolved = std::move(rv);
            resp.confirmation = std::move(cf);
            callback(json_response(drogon::k200OK, resp.to_json()));
            return;
        }

        // 2~5 건 — 분기 질문
        schemas::OnboardingNormalizeResponse resp;
        resp.state = schemas::OnboardingState::NEED_DISAMBIGUATION;
        resp.round = reqobj.round;
        resp.candidates_count = static_cast<int>(rows.size());

        std::string options_label_text;
        for (auto row : rows) {
            schemas::OnboardingCandidate c;
            c.item_code = row["item_code"].as<std::string>();
            c.item_name = row["drug_name"].as<std::string>();
            if (!row["ingredient_name"].isNull())
                c.ingredient_name = row["ingredient_name"].as<std::string>();
            if (!row["classification_name"].isNull())
                c.classification_name = row["classification_name"].as<std::string>();
            if (!row["manufacturer"].isNull())
                c.manufacturer = row["manufacturer"].as<std::string>();
            // 단순 hint — 약명 그대로
            c.hint = c.classification_name.value_or("");
            resp.candidates.push_back(c);
        }

        schemas::OnboardingQuestion q;
        q.field = "candidate_pick";
        q.text  = "비슷한 이름의 약이 여러 개 있어요. 어떤 약 드시나요?";
        q.tts_text = "비슷한 이름 약이 여러 개 있어요. 어떤 거 드세요?";
        for (const auto& c : resp.candidates) {
            schemas::OnboardingOption o;
            o.value = c.item_code;
            o.label = c.item_name;
            q.options.push_back(o);
        }
        q.choice_token = random_token();
        resp.question = q;

        callback(json_response(drogon::k200OK, resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

// =====================================================
// GET /v1/pill/pool — 사용자 약 풀 조회 (실 DB)
// =====================================================
void Pill::handle_pool_list(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    const auto anon_opt = resolve_anonymous_id(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    const std::string include_inactive = req->getParameter("include_inactive");
    const bool with_inactive = (include_inactive == "true" || include_inactive == "1");

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        std::string sql =
            "SELECT ump.pool_id, ump.item_code, p.drug_name, ump.reg_method, "
            "       ump.is_active, ump.created_at, ump.user_category, p.classification_name "
            "FROM user_medication_pool ump "
            "JOIN pill_identification p ON p.item_code = ump.item_code "
            "WHERE ump.anonymous_id = ? ";
        if (!with_inactive) sql += "AND ump.is_active = TRUE ";
        sql += "ORDER BY ump.created_at DESC";

        auto rows = db->execSqlSync(sql, *anon_opt);

        schemas::PoolListResponse resp;
        for (auto row : rows) {
            schemas::PoolItem it;
            it.pool_id    = row["pool_id"].as<int>();
            it.item_code  = row["item_code"].as<std::string>();
            it.drug_name  = row["drug_name"].as<std::string>();
            it.reg_method = row["reg_method"].as<std::string>();
            it.is_active  = row["is_active"].as<int>() == 1;
            it.created_at = row["created_at"].as<std::string>();
            if (!row["user_category"].isNull())
                it.user_category = row["user_category"].as<std::string>();
            if (!row["classification_name"].isNull())
                it.classification_name = row["classification_name"].as<std::string>();
            resp.items.push_back(std::move(it));
        }
        resp.total_count = static_cast<int>(resp.items.size());
        callback(json_response(drogon::k200OK, resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

// =====================================================
// POST /v1/pill/pool — 약 풀에 추가
// =====================================================
void Pill::handle_pool_add(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    const auto anon_opt = resolve_anonymous_id(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::PoolAddRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드 누락 또는 형식 오류", err_field));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        // item_code 존재 검증
        auto exist = db->execSqlSync(
            "SELECT drug_name, classification_name FROM pill_identification WHERE item_code = ? LIMIT 1",
            reqobj.item_code);
        if (exist.empty()) {
            callback(error_response(drogon::k404NotFound, "ITEM_CODE_NOT_FOUND", "존재하지 않는 품목코드입니다."));
            return;
        }

        // 활성 중복 검사
        auto dup = db->execSqlSync(
            "SELECT pool_id FROM user_medication_pool "
            "WHERE anonymous_id = ? AND item_code = ? AND is_active = TRUE LIMIT 1",
            *anon_opt, reqobj.item_code);
        if (!dup.empty()) {
            callback(error_response(drogon::k409Conflict, "ALREADY_IN_POOL", "이미 등록된 약입니다."));
            return;
        }

        // 자동 매핑 — user_category 미입력 시 classification_name 사용
        std::string user_category = reqobj.user_category.value_or(
            exist[0]["classification_name"].isNull() ? std::string{}
                                                     : exist[0]["classification_name"].as<std::string>());

        auto ins = db->execSqlSync(
            "INSERT INTO user_medication_pool "
            "(anonymous_id, item_code, reg_method, user_category, is_active) "
            "VALUES (?, ?, ?, ?, TRUE)",
            *anon_opt, reqobj.item_code, reqobj.reg_method, user_category);

        // 새로 INSERT 된 행 다시 조회
        auto rows = db->execSqlSync(
            "SELECT ump.pool_id, ump.item_code, p.drug_name, ump.reg_method, "
            "       ump.is_active, ump.created_at, ump.user_category, p.classification_name "
            "FROM user_medication_pool ump "
            "JOIN pill_identification p ON p.item_code = ump.item_code "
            "WHERE ump.anonymous_id = ? AND ump.item_code = ? "
            "ORDER BY ump.pool_id DESC LIMIT 1",
            *anon_opt, reqobj.item_code);

        if (rows.empty()) {
            callback(error_response(drogon::k500InternalServerError, "INSERT_FAILED", "등록 후 조회 실패."));
            return;
        }
        auto row = rows[0];
        schemas::PoolItem it;
        it.pool_id    = row["pool_id"].as<int>();
        it.item_code  = row["item_code"].as<std::string>();
        it.drug_name  = row["drug_name"].as<std::string>();
        it.reg_method = row["reg_method"].as<std::string>();
        it.is_active  = true;
        it.created_at = row["created_at"].as<std::string>();
        if (!row["user_category"].isNull())
            it.user_category = row["user_category"].as<std::string>();
        if (!row["classification_name"].isNull())
            it.classification_name = row["classification_name"].as<std::string>();

        callback(json_response(drogon::k201Created, it.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

// =====================================================
// DELETE /v1/pill/pool/{pool_id} — 개별 soft delete
// =====================================================
void Pill::handle_pool_remove(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              int pool_id)
{
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    const auto anon_opt = resolve_anonymous_id(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        // 본인 소유 검증
        auto own = db->execSqlSync(
            "SELECT anonymous_id FROM user_medication_pool WHERE pool_id = ? LIMIT 1",
            pool_id);
        if (own.empty()) {
            callback(error_response(drogon::k404NotFound, "POOL_NOT_FOUND", "존재하지 않는 약 풀입니다."));
            return;
        }
        if (own[0]["anonymous_id"].as<std::string>() != *anon_opt) {
            callback(error_response(drogon::k403Forbidden, "FORBIDDEN", "본인 데이터가 아닙니다."));
            return;
        }
        db->execSqlSync(
            "UPDATE user_medication_pool "
            "SET is_active = FALSE, deactivated_at = NOW() "
            "WHERE pool_id = ?",
            pool_id);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        callback(resp);
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

// =====================================================
// DELETE /v1/pill/pool/all — 전체 리셋 (사용자 명시 트리거만)
// =====================================================
void Pill::handle_pool_reset(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    const auto confirm = req->getHeader("X-Confirm-Reset");
    if (confirm != "true") {
        callback(error_response(drogon::k400BadRequest, "MISSING_CONFIRM_HEADER",
                                "X-Confirm-Reset: true 헤더가 필요합니다."));
        return;
    }
    const auto user_id_opt = auth_user_id(req);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    const auto anon_opt = resolve_anonymous_id(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }
    try {
        db->execSqlSync(
            "UPDATE user_medication_pool "
            "SET is_active = FALSE, deactivated_at = NOW() "
            "WHERE anonymous_id = ? AND is_active = TRUE",
            *anon_opt);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        callback(resp);
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

} // namespace medibridge::routers
