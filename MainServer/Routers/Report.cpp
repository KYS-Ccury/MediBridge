// =====================================================
// Report Router — 실 DB 조회 + 기본 JSON/HTML 응답
// =====================================================
// 본 단계: format=json 만 완전 동작. format=pdf/html 은 추후 ReportPdfRenderer
// 가 채워지면 활성화 (현재는 HTML 단순 출력으로 임시 대체).
// =====================================================
#include "Report.h"
#include "../Schemas/ReportSchema.h"
#include "../Database/Connection.h"
#include "../Services/Auth/JwtIssuer.h"
#include "../Services/Report/ReportHtmlRenderer.h"
#include "../Services/Report/ReportPdfRenderer.h"
#include "../Threading/WorkerPool.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>
#include <unordered_map>

using medibridge::database::Connection;
namespace AuthSvc = medibridge::services::auth;
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
    auto user_id_opt = AuthSvc::JwtIssuer::extract_user_id_from_header(h);
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
        //
        // ⭐ 2026-05-15 버그 수정: 기존 switch(case 1-4) 는 5개 이상 약 종류 시 default 분기로
        //    codes[0] 만 bind 되어 SQL placeholder 미스매치 (`?,?,?,?` 안 채워짐).
        //    item_code 는 DB 의 PK 라 alphanumeric only — SQL injection 위험 없음.
        //    placeholder ? 대신 quote 한 리터럴을 직접 SQL 에 박아 N 약 종류 지원.
        if (!agg.empty()) {
            std::string in_clause = "(";
            int i = 0;
            for (auto& kv : agg) {
                if (i++) in_clause += ",";
                // alphanumeric PK — escape 불필요. 그래도 방어적으로 따옴표 이중 보호:
                std::string safe = kv.first;
                // ' → '' 치환 (defense in depth)
                size_t pos = 0;
                while ((pos = safe.find('\'', pos)) != std::string::npos) {
                    safe.replace(pos, 1, "''");
                    pos += 2;
                }
                in_clause += "'" + safe + "'";
            }
            in_clause += ")";
            std::string side_sql =
                "SELECT p.item_code, p.drug_name, "
                "       d.caution_text, d.side_effect_text "
                "FROM pill_identification p "
                "LEFT JOIN drug_overview d ON d.item_code = p.item_code "
                "WHERE p.item_code IN " + in_clause;

            orm::Result side_rows = db->execSqlSync(side_sql);
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
        // Drogon 1.8.7 SqlBinder 는 const-rvalue std::string 을 인식 못해
        // 미리 lvalue 변수로 풀어서 바인딩.
        try {
            const std::string period_start = from_date.empty() ? std::string("1970-01-01") : from_date;
            const std::string period_end   = to_date.empty()   ? std::string("9999-12-31") : to_date;
            const std::string consumed_str = resp.consumed_summary.to_json().toStyledString();
            const std::string side_str     = Json::Value(Json::arrayValue).toStyledString();
            db->execSqlSync(
                "INSERT INTO user_reports "
                "(report_id, anonymous_id, period_start, period_end, "
                " consumed_summary, side_effect_quotes) "
                "VALUES (?, ?, ?, ?, ?, ?)",
                resp.report_id, anon_id,
                period_start, period_end,
                consumed_str, side_str);
        } catch (const orm::DrogonDbException&) {
            // 보고서 행 INSERT 실패는 응답 자체엔 영향 없음 (감사 로그 차원)
        }

        if (format == "html") {
            const auto html_body =
                medibridge::services::report::ReportHtmlRenderer::render(resp);
            auto http = drogon::HttpResponse::newHttpResponse();
            http->setStatusCode(drogon::k200OK);
            http->setContentTypeCode(drogon::CT_TEXT_HTML);
            http->setBody(html_body);
            callback(http);
            return;
        }
        if (format == "pdf") {
            // HTML 먼저 생성 후, PDF 변환은 WorkerPool 에 위임 (Drogon 핸들러 스레드 차단 방지)
            const auto html_body =
                medibridge::services::report::ReportHtmlRenderer::render(resp);
            const auto report_id = resp.report_id;

            medibridge::threading::WorkerPool::instance().submit(
                [html_body, report_id, callback]() mutable {
                    auto pdf_bytes =
                        medibridge::services::report::ReportPdfRenderer::render(html_body);
                    if (pdf_bytes.empty()) {
                        Json::Value body, err;
                        err["code"]    = "PDF_RENDER_FAILED";
                        err["message"] = "PDF 변환 실패 — wkhtmltopdf 미설치 또는 변환 오류.";
                        body["error"] = err;
                        auto http = drogon::HttpResponse::newHttpJsonResponse(body);
                        http->setStatusCode(drogon::k500InternalServerError);
                        callback(http);
                        return;
                    }
                    auto http = drogon::HttpResponse::newHttpResponse();
                    http->setStatusCode(drogon::k200OK);
                    http->setContentTypeString("application/pdf");
                    http->addHeader("Content-Disposition",
                                    "inline; filename=\"medibridge_" + report_id + ".pdf\"");
                    http->setBody(std::string(
                        reinterpret_cast<const char*>(pdf_bytes.data()),
                        pdf_bytes.size()));
                    callback(http);
                });
            return;   // 응답은 워커 콜백에서
        }
        // 기본 JSON
        callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

} // namespace medibridge::routers
