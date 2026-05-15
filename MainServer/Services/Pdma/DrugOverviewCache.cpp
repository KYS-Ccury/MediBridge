// =====================================================
// DrugOverviewCache — 구현 (TTL 30일 동기 refresh, 2026-05-15 회귀)
// =====================================================
// 정책 (v3 — 30일 TTL 회귀):
//   1) DB SELECT (cached_at 포함)
//   2) 미스          → PdmaApiClient HTTPS GET → DB UPSERT → 반환
//   3) hit + fresh   → DB 응답 (외부 호출 없음)
//   4) hit + stale   → PdmaApiClient HTTPS GET → DB UPSERT → 반환  ⭐
//   주기 갱신은 CSV 일괄 적재 (import_pdma.py) 로 별도 수행 가능.
//
// TTL 기본 30일. 환경변수 MEDIBRIDGE_PDMA_CACHE_TTL_DAYS 로 변경 (0 = 무한).
// =====================================================
#include "DrugOverviewCache.h"
#include "PdmaApiClient.h"
#include "../../Database/Connection.h"

#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace orm = drogon::orm;

namespace medibridge::services::pdma {

namespace {

/// drug_overview row → Json::Value (캐시 응답 형식)
Json::Value row_to_json(const orm::Row& row)
{
    Json::Value v;
    auto str_or_empty = [&row](const char* col) -> std::string {
        return row[col].isNull() ? std::string{} : row[col].as<std::string>();
    };
    v["item_code"]        = row["item_code"].as<std::string>();
    v["efficacy_text"]    = str_or_empty("efficacy_text");
    v["usage_text"]       = str_or_empty("usage_text");
    v["warning_text"]     = str_or_empty("warning_text");
    v["caution_text"]     = str_or_empty("caution_text");
    v["interaction_text"] = str_or_empty("interaction_text");
    v["side_effect_text"] = str_or_empty("side_effect_text");
    v["storage_text"]     = str_or_empty("storage_text");
    return v;
}

/// PDMA 응답 필드명 → DB 컬럼 매핑 (e약은요 API 표준 키)
std::string extract(const Json::Value& src, const char* key)
{
    return src.isMember(key) && src[key].isString() ? src[key].asString() : "";
}

/// 환경변수에서 TTL 일수 읽기 — 기본 30, 0 = 무한
int ttl_days()
{
    static int cached = []() {
        if (const char* env = std::getenv("MEDIBRIDGE_PDMA_CACHE_TTL_DAYS")) {
            try {
                int n = std::stoi(env);
                if (n >= 0) return n;
            } catch (...) {}
        }
        return 30;
    }();
    return cached;
}

} // anonymous

std::optional<std::pair<Json::Value, int>>
DrugOverviewCache::get_from_db_with_age(const std::string& item_code)
{
    auto db = medibridge::database::Connection::instance().client();
    if (!db) return std::nullopt;

    try {
        // DATEDIFF(NOW(), cached_at) = 경과 일수
        auto rows = db->execSqlSync(
            "SELECT item_code, efficacy_text, usage_text, warning_text, "
            "       caution_text, interaction_text, side_effect_text, storage_text, "
            "       DATEDIFF(NOW(), cached_at) AS age_days "
            "FROM drug_overview WHERE item_code = ? LIMIT 1",
            item_code);
        if (rows.empty()) return std::nullopt;
        const int age = rows[0]["age_days"].isNull()
                      ? 0
                      : rows[0]["age_days"].as<int>();
        return std::make_pair(row_to_json(rows[0]), age);
    } catch (const orm::DrogonDbException& e) {
        std::cerr << "[DrugOverviewCache] SQL error: " << e.base().what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Json::Value>
DrugOverviewCache::get_or_fetch(const std::string& item_code)
{
    const int ttl = ttl_days();
    auto cached   = get_from_db_with_age(item_code);

    // 1) hit fresh — 즉시 반환
    if (cached.has_value()) {
        const int age = cached->second;
        if (ttl == 0 || age < ttl) {
            return cached->first;
        }
        std::cout << "[DrugOverviewCache] stale (age=" << age
                  << " days, ttl=" << ttl << ") — refresh trigger: "
                  << item_code << std::endl;
    }

    // 2) miss 또는 stale — 외부 API 비동기 fetch 트리거.
    //    다음 호출에 fresh hit. 즉시 동기 응답 필요한 라우터는 직접
    //    PdmaApiClient::fetch_drug_overview() + cache() 체이닝.
    PdmaApiClient::instance().fetch_drug_overview(item_code,
        [item_code](const Json::Value& resp, int status) {
            if (status == 200 && !resp.isNull()) {
                cache(item_code, resp);
            }
        });

    // stale 인 경우 — 새 응답이 올 때까지는 stale 본문이라도 반환 (사용성 우선).
    // miss 인 경우 — nullopt 로 클라가 재시도하게 함.
    if (cached.has_value()) return cached->first;
    return std::nullopt;
}

void DrugOverviewCache::cache(const std::string& item_code, const Json::Value& pdma_data)
{
    auto db = medibridge::database::Connection::instance().client();
    if (!db) return;

    const std::string efficacy    = extract(pdma_data, "efcyQesitm");
    const std::string usage       = extract(pdma_data, "useMethodQesitm");
    const std::string warning     = extract(pdma_data, "atpnWarnQesitm");
    const std::string caution     = extract(pdma_data, "atpnQesitm");
    const std::string interaction = extract(pdma_data, "intrcQesitm");
    const std::string side_effect = extract(pdma_data, "seQesitm");
    const std::string storage     = extract(pdma_data, "depositMethodQesitm");

    try {
        db->execSqlSync(
            "INSERT INTO drug_overview "
            "(item_code, efficacy_text, usage_text, warning_text, "
            " caution_text, interaction_text, side_effect_text, storage_text) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
            "ON DUPLICATE KEY UPDATE "
            "  efficacy_text=VALUES(efficacy_text), "
            "  usage_text=VALUES(usage_text), "
            "  warning_text=VALUES(warning_text), "
            "  caution_text=VALUES(caution_text), "
            "  interaction_text=VALUES(interaction_text), "
            "  side_effect_text=VALUES(side_effect_text), "
            "  storage_text=VALUES(storage_text), "
            "  cached_at=NOW()",
            item_code, efficacy, usage, warning, caution, interaction, side_effect, storage);
        std::cout << "[DrugOverviewCache] cached item_code=" << item_code << std::endl;
    } catch (const orm::DrogonDbException& e) {
        std::cerr << "[DrugOverviewCache] INSERT 실패: " << e.base().what() << std::endl;
    }
}

} // namespace medibridge::services::pdma
