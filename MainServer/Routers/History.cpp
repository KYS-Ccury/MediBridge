// =====================================================
// History Router — 실 DB CRUD (TestMode 와 무관, 항상 실 동작)
// =====================================================
#include "History.h"
#include "../Schemas/HistorySchema.h"
#include "../Database/Connection.h"
#include "../Services/TestMode/MockAuth.h"
#include "../Utils/TimeUtil.h"

#include <drogon/HttpResponse.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <random>
#include <sstream>

using medibridge::database::Connection;
namespace TestMode = medibridge::testmode;
namespace orm = drogon::orm;

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

static std::string gen_intake_id()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::stringstream ss;
    ss << "intake_" << std::hex << gen();
    return ss.str();
}

static std::optional<std::string> resolve_anon(const std::string& user_id)
{
    auto db = Connection::instance().client();
    if (!db) return std::nullopt;
    try {
        auto r = db->execSqlSync(
            "SELECT anonymous_id FROM pseudonym_map "
            "WHERE user_id = ? AND severed_at IS NULL LIMIT 1", user_id);
        if (r.empty()) return std::nullopt;
        return r[0]["anonymous_id"].as<std::string>();
    } catch (const orm::DrogonDbException&) {
        return std::nullopt;
    }
}

/// HH 시각 → 아침/점심/저녁/취침
static std::string time_slot_from_datetime(const std::string& dt)
{
    // dt: "YYYY-MM-DD HH:MM:SS"
    if (dt.size() < 13) return "기타";
    int hh = 0;
    try { hh = std::stoi(dt.substr(11, 2)); } catch (...) { return "기타"; }
    if (hh >=  5 && hh < 11) return "아침";
    if (hh >= 11 && hh < 16) return "점심";
    if (hh >= 16 && hh < 21) return "저녁";
    return "취침";
}

void History::handle_record(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto h = req->getHeader("Authorization");
    auto user_id_opt = TestMode::extract_user_id_from_bearer(h);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    auto anon_opt = resolve_anon(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    auto json_ptr = req->getJsonObject();
    if (!json_ptr) {
        callback(error_response(drogon::k400BadRequest, "INVALID_JSON", "JSON 본문이 아닙니다."));
        return;
    }
    auto reqobj = schemas::HistoryRecordRequest::from_json(*json_ptr);
    std::string err_field, err_code;
    if (!reqobj.is_valid(err_field, err_code)) {
        callback(error_response(drogon::k400BadRequest, err_code, "필수 필드 검증 실패", err_field));
        return;
    }

    auto db = Connection::instance().client();
    try {
        // item_code 존재 검증
        auto pill = db->execSqlSync(
            "SELECT drug_name FROM pill_identification WHERE item_code = ? LIMIT 1",
            reqobj.item_code);
        if (pill.empty()) {
            callback(error_response(drogon::k404NotFound, "ITEM_CODE_NOT_FOUND",
                                    "존재하지 않는 품목코드입니다.", "item_code"));
            return;
        }

        const std::string intake_id = gen_intake_id();
        const std::string memo      = reqobj.memo.value_or("");

        if (reqobj.intake_datetime.empty()) {
            db->execSqlSync(
                "INSERT INTO medication_intake_logs "
                "(intake_id, anonymous_id, item_code, intake_datetime, quantity, memo, confidence_score) "
                "VALUES (?, ?, ?, NOW(), ?, ?, ?)",
                intake_id, *anon_opt, reqobj.item_code,
                reqobj.quantity, memo,
                reqobj.confidence_score.has_value() ? *reqobj.confidence_score : -1.0);
        } else {
            db->execSqlSync(
                "INSERT INTO medication_intake_logs "
                "(intake_id, anonymous_id, item_code, intake_datetime, quantity, memo, confidence_score) "
                "VALUES (?, ?, ?, ?, ?, ?, ?)",
                intake_id, *anon_opt, reqobj.item_code, reqobj.intake_datetime,
                reqobj.quantity, memo,
                reqobj.confidence_score.has_value() ? *reqobj.confidence_score : -1.0);
        }

        auto rows = db->execSqlSync(
            "SELECT intake_datetime, created_at FROM medication_intake_logs WHERE intake_id = ?",
            intake_id);

        schemas::HistoryRecordResponse resp;
        resp.intake_id       = intake_id;
        resp.user_id         = *user_id_opt;
        resp.item_code       = reqobj.item_code;
        resp.quantity        = reqobj.quantity;
        resp.memo            = memo;
        if (!rows.empty()) {
            resp.intake_datetime = rows[0]["intake_datetime"].as<std::string>();
            resp.created_at      = rows[0]["created_at"].as<std::string>();
        }
        auto http = drogon::HttpResponse::newHttpJsonResponse(resp.to_json());
        http->setStatusCode(drogon::k201Created);
        callback(http);
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

void History::handle_list(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto h = req->getHeader("Authorization");
    auto user_id_opt = TestMode::extract_user_id_from_bearer(h);
    if (!user_id_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "인증 실패."));
        return;
    }
    auto anon_opt = resolve_anon(*user_id_opt);
    if (!anon_opt) {
        callback(error_response(drogon::k401Unauthorized, "INVALID_TOKEN", "사용자 식별 실패."));
        return;
    }

    int page = 1, page_size = 20;
    try { auto p = req->getParameter("page");      if (!p.empty()) page = std::stoi(p); } catch (...) {}
    try { auto p = req->getParameter("page_size"); if (!p.empty()) page_size = std::stoi(p); } catch (...) {}
    if (page < 1) page = 1;
    if (page_size < 1 || page_size > 100) page_size = 20;
    const std::string sort_order = (req->getParameter("sort_order") == "asc") ? "ASC" : "DESC";

    const std::string from_date = req->getParameter("from_date");
    const std::string to_date   = req->getParameter("to_date");
    const std::string item_code = req->getParameter("item_code");

    auto db = Connection::instance().client();
    try {
        std::string sql =
            "SELECT il.intake_id, il.item_code, p.drug_name, il.intake_datetime, "
            "       il.quantity, il.memo "
            "FROM medication_intake_logs il "
            "JOIN pill_identification p ON p.item_code = il.item_code "
            "WHERE il.anonymous_id = ? ";
        std::vector<std::string> params{*anon_opt};
        if (!from_date.empty()) { sql += "AND il.intake_datetime >= ? "; params.push_back(from_date); }
        if (!to_date.empty())   { sql += "AND il.intake_datetime <= ? "; params.push_back(to_date); }
        if (!item_code.empty()) { sql += "AND il.item_code = ? ";        params.push_back(item_code); }
        sql += "ORDER BY il.intake_datetime " + sort_order
             + " LIMIT " + std::to_string(page_size)
             + " OFFSET " + std::to_string((page - 1) * page_size);

        orm::Result rows = [&]() {
            switch (params.size()) {
                case 1: return db->execSqlSync(sql, params[0]);
                case 2: return db->execSqlSync(sql, params[0], params[1]);
                case 3: return db->execSqlSync(sql, params[0], params[1], params[2]);
                default: return db->execSqlSync(sql, params[0], params[1], params[2], params[3]);
            }
        }();

        // 총 개수 (pagination 헤더용)
        std::string count_sql = "SELECT COUNT(*) AS cnt FROM medication_intake_logs WHERE anonymous_id = ?";
        std::vector<std::string> cparams{*anon_opt};
        if (!from_date.empty()) { count_sql += " AND intake_datetime >= ?"; cparams.push_back(from_date); }
        if (!to_date.empty())   { count_sql += " AND intake_datetime <= ?"; cparams.push_back(to_date); }
        if (!item_code.empty()) { count_sql += " AND item_code = ?";        cparams.push_back(item_code); }

        orm::Result cr = [&]() {
            switch (cparams.size()) {
                case 1: return db->execSqlSync(count_sql, cparams[0]);
                case 2: return db->execSqlSync(count_sql, cparams[0], cparams[1]);
                case 3: return db->execSqlSync(count_sql, cparams[0], cparams[1], cparams[2]);
                default: return db->execSqlSync(count_sql, cparams[0], cparams[1], cparams[2], cparams[3]);
            }
        }();
        const int total = cr.empty() ? 0 : cr[0]["cnt"].as<int>();

        schemas::HistoryListResponse resp;
        for (auto row : rows) {
            schemas::HistoryItem it;
            it.intake_id       = row["intake_id"].as<std::string>();
            it.item_code       = row["item_code"].as<std::string>();
            it.drug_name       = row["drug_name"].as<std::string>();
            it.intake_datetime = row["intake_datetime"].as<std::string>();
            it.quantity        = row["quantity"].as<int>();
            it.memo            = row["memo"].isNull() ? std::string{} : row["memo"].as<std::string>();
            it.time_slot       = time_slot_from_datetime(it.intake_datetime);
            resp.items.push_back(std::move(it));
        }
        resp.page        = page;
        resp.page_size   = page_size;
        resp.total_count = total;
        resp.total_pages = (total + page_size - 1) / page_size;

        callback(drogon::HttpResponse::newHttpJsonResponse(resp.to_json()));
    } catch (const orm::DrogonDbException& e) {
        callback(error_response(drogon::k500InternalServerError, "DB_ERROR", e.base().what()));
    }
}

} // namespace medibridge::routers
