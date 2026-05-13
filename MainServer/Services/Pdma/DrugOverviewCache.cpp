// =====================================================
// DrugOverviewCache — 구현
// =====================================================
// 정책:
//   1차 DB SELECT → 미스 시 PdmaApiClient HTTPS GET → DB INSERT.
//   TTL 정책 없음 — 식약처 본문은 자주 안 바뀜. 갱신은 CSV 일괄 적재 (import_pdma.py) 로.
//
// 시그니처:
//   get_from_db()    — DB 만 (동기). 빠름.
//   get_or_fetch()   — 캐시 hit 만 즉시 반환. 미스 시 백그라운드 fetch + std::nullopt.
//                       (즉, 다음 호출 시 캐시 hit 되도록 채워두는 용도)
//   cache()          — 외부 API 응답을 DB 에 UPSERT (PdmaApiClient 콜백에서 호출).
// =====================================================
#include "DrugOverviewCache.h"
#include "PdmaApiClient.h"
#include "../../Database/Connection.h"

#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <iostream>

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

} // anonymous

std::optional<Json::Value>
DrugOverviewCache::get_from_db(const std::string& item_code)
{
    auto db = medibridge::database::Connection::instance().client();
    if (!db) return std::nullopt;

    try {
        auto rows = db->execSqlSync(
            "SELECT item_code, efficacy_text, usage_text, warning_text, "
            "       caution_text, interaction_text, side_effect_text, storage_text "
            "FROM drug_overview WHERE item_code = ? LIMIT 1",
            item_code);
        if (rows.empty()) return std::nullopt;
        return row_to_json(rows[0]);
    } catch (const orm::DrogonDbException& e) {
        std::cerr << "[DrugOverviewCache] SQL error: " << e.base().what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Json::Value>
DrugOverviewCache::get_or_fetch(const std::string& item_code)
{
    // 1) 캐시 hit
    if (auto cached = get_from_db(item_code); cached.has_value()) return cached;

    // 2) 미스 — 백그라운드 fetch 트리거 (다음 호출 시 hit 되도록).
    //    호출 즉시 결과 필요한 경우엔 라우터 단에서 PdmaApiClient::fetch_drug_overview
    //    + cache() 직접 콜백 체이닝.
    PdmaApiClient::instance().fetch_drug_overview(item_code,
        [item_code](const Json::Value& resp, int status) {
            if (status == 200 && !resp.isNull()) {
                cache(item_code, resp);
            }
        });
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
