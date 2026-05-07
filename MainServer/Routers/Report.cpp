// =====================================================
// Report Router — 실 DB 조회 + 기본 JSON/HTML 응답
// =====================================================
// 본 단계: format=json 만 완전 동작. format=pdf/html 은 추후 ReportPdfRenderer
// 가 채워지면 활성화 (현재는 HTML 단순 출력으로 임시 대체).
// =====================================================
#include "Report.h"
#include "../Schemas/ReportSchema.h"
#include "../Database/Connection.h"
#include "../Services/TestMode/MockAuth.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>
#include <unordered_map>

using medibridge::database::Connection;
namespace TestMode = medibridge::testmode;
namespace orm = drogon::orm;

namespace medibridge::routers {

static drogon::HttpResponsePtr error_response(drogon::HttpStatusCode code,
                                              const std::string& err_code,
                                              const std::string& message)
{
    Json::Value body, err;
    err["code"] = err_code; err["message"] = message;
    body["error"] = err;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(code);
    return resp;
}

static std::string gen_report_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << "report_" << std::hex << gen();
    return ss.str();
}

void Report::handle_generate(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto h = req->getHeader("Authorization");
    auto user_id_opt = TestMode::extract_user_id_from_bearer(h);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }

    auto db = Connection::instance().client();
    if (!db) {
        callback(error_response(drogon::k503ServiceUnavailable, "DB_UNAVAILABLE", "DB 미준비."));
        return;
    }

    // anonymous_id + user 정보
    std::string anon_id, user_name, email;
    try {
        auto urows = db->execSqlSync(
            "SELECT u.user_name, u.email, pm.anonymous_id "
            "FROM users u "
            "JOIN pseudonym_map pm ON pm.user_id = u.user_id "
            "WHERE u.user_id = ? AND pm.severed_at IS NULL LIMIT 1",
            *user_id_opt);
        if (urows.empty()) {
            callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
            return;
        }
        user_name = urows[0]["user_name"].as<std::string>();
        email     = urows[0]["email"].as<std::string>();
        anon_id   = urows[0]["anonymous_id"].as<std::string>();
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
        return;
    }

    const std::string from_date = req->getParameter("from_date");
    const std::string to_date   = req->getParameter("to_date");
    const std::string format    = req->getParameter("format");  // json / html / pdf

    schemas::ReportResponse resp;
    resp.report_id    = gen_report_id();
    resp.period.from_date = from_date.empty() ? "" : from_date;
    resp.period.to_date   = to_date.empty()   ? "" : to_date;
    resp.generated_at = utils::current_iso8601_utc();
    Json::Value u;
    u["user_id"]   = *user_id_opt;
    u["email"]     = email;
    u["user_name"] = user_name;
    resp.user = u;

    try {
        // intake logs (period 적용 — 빈 값이면 전체)
        std::string sql =
            "SELECT il.intake_datetime, il.quantity, il.memo, "
            "       p.drug_name, p.item_code "
            "FROM medication_intake_logs il "
            "JOIN pill_identification p ON p.item_code = il.item_code "
            "WHERE il.anonymous_id = ? ";
        std::vector<std::string> params{anon_id};
        if (!from_date.empty()) { sql += "AND il.intake_datetime >= ? "; params.push_back(from_date); }
        if (!to_date.empty())   { sql += "AND il.intake_datetime <= ? "; params.push_back(to_date); }
        sql += "ORDER BY il.intake_datetime ASC";

        orm::Result rows = [&]() {
            switch (params.size()) {
                case 1: return db->execSqlSync(sql, params[0]);
                case 2: return db->execSqlSync(sql, params[0], params[1]);
                default: return db->execSqlSync(sql, params[0], params[1], params[2]);
            }
        }();

        std::unordered_map<std::string, schemas::ConsumedByDrug> agg;
        for (auto row : rows) {
            schemas::IntakeLogEntry e;
            e.intake_datetime = row["intake_datetime"].as<std::string>();
            e.drug_name       = row["drug_name"].as<std::string>();
            e.quantity        = row["quantity"].as<int>();
            e.memo            = row["memo"].isNull() ? std::string{} : row["memo"].as<std::string>();
            resp.intake_logs.push_back(e);

            const auto code = row["item_code"].as<std::string>();
            auto& d = agg[code];
            if (d.item_code.empty()) {
                d.item_code = code;
                d.drug_name = e.drug_name;
                d.first_intake = e.intake_datetime;
            }
            d.last_intake     = e.intake_datetime;
            d.total_quantity += e.quantity;
        }
        for (auto& kv : agg) resp.consumed_summary.by_drug.push_back(kv.second);
        resp.consumed_summary.total_intakes = static_cast<int>(rows.size());

        // 부작용 인용 (식약처 e약은요 그대로 인용 — LLM 변환 X)
        if (!agg.empty()) {
            std::string in_clause = "(";
            std::vector<std::string> codes;
            int i = 0;
            for (auto& kv : agg) {
                if (i++) in_clause += ",";
                in_clause += "?";
                codes.push_back(kv.first);
            }
            in_clause += ")";
            std::string side_sql =
                "SELECT p.item_code, p.drug_name, "
                "       d.caution_text, d.side_effect_text "
                "FROM pill_identification p "
                "LEFT JOIN drug_overview d ON d.item_code = p.item_code "
                "WHERE p.item_code IN " + in_clause;

            orm::Result side_rows = [&]() {
                switch (codes.size()) {
                    case 1: return db->execSqlSync(side_sql, codes[0]);
                    case 2: return db->execSqlSync(side_sql, codes[0], codes[1]);
                    case 3: return db->execSqlSync(side_sql, codes[0], codes[1], codes[2]);
                    case 4: return db->execSqlSync(side_sql, codes[0], codes[1], codes[2], codes[3]);
                    default: return db->execSqlSync(side_sql, codes[0]);
                }
            }();
            for (auto row : side_rows) {
                schemas::SideEffectQuote q;
                q.item_code        = row["item_code"].as<std::string>();
                q.drug_name        = row["drug_name"].as<std::string>();
                q.caution_text     = row["caution_text"].isNull()     ? std::string{} : row["caution_text"].as<std::string>();
                q.side_effect_text = row["side_effect_text"].isNull() ? std::string{} : row["side_effect_text"].as<std::string>();
                q.source_note      = "본 정보는 식약처 e약은요 본문 그대로 인용한 것이며, 약사·의사 상담을 권유드립니다.";
                resp.side_effect_quotes.push_back(q);
            }
        }

        // INSERT 보고서 행 (이력 추적용)
        try {
            db->execSqlSync(
                "INSERT INTO user_reports "
                "(report_id, anonymous_id, period_start, period_end, "
                " consumed_summary, side_effect_quotes) "
                "VALUES (?, ?, ?, ?, ?, ?)",
                resp.report_id, anon_id,
                from_date.empty() ? std::string("1970-01-01") : from_date,
                to_date.empty()   ? std::string("9999-12-31") : to_date,
                resp.consumed_summary.to_json().toStyledString(),
                Json::Value(Json::arrayValue).toStyledString());
        } catch (const orm::DrogonDbException&) {
            // 보고서 행 INSERT 실패는 응답 자체엔 영향 없음 (감사 로그 차원)
        }

        if (format == "html") {
            std::ostringstream html;
            html << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>메디브릿지 보고서</title></head><body>";
            html << "<h1>메디브릿지 복약 보고서</h1>";
            html << "<p>기간: " << resp.period.from_date << " ~ " << resp.period.to_date << "</p>";
            html << "<h2>총 복용 " << resp.consumed_summary.total_intakes << "회</h2>";
            html << "<table border='1' cellpadding='6'><tr><th>일시</th><th>약</th><th>개수</th><th>메모</th></tr>";
            for (const auto& e : resp.intake_logs) {
                html << "<tr><td>" << e.intake_datetime << "</td><td>" << e.drug_name
                     << "</td><td>" << e.quantity << "</td><td>" << e.memo << "</td></tr>";
            }
            html << "</table>";
            html << "<h2>식약처 e약은요 인용 — 약사·의사 상담을 권유드립니다.</h2><ul>";
            for (const auto& q : resp.side_effect_quotes) {
                html << "<li><b>" << q.drug_name << "</b><br/>주의: " << q.caution_text
                     << "<br/>부작용: " << q.side_effect_text << "</li>";
            }
            html << "</ul></body></html>";
            auto http = drogon::HttpResponse::newHttpResponse();
            http->setStatusCode(drogon::k200OK);
            http->setContentTypeCode(drogon::CT_TEXT_HTML);
            http->setBody(html.str());
            callback(http);
            return;
        }
        // 기본 JSON
        callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

} // namespace medibridge::routers
