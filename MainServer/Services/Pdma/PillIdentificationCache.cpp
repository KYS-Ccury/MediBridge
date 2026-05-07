#include "PillIdentificationCache.h"
#include "../../Database/Connection.h"

namespace medibridge::services::pdma {

std::optional<Json::Value>
PillIdentificationCache::get_by_item_code(const std::string& item_code)
{
    // TODO (영역 B 분담) — ⚠ SQL 인젝션 방어 절대 준수:
    //
    // ✅ Drogon ORM (자동 prepared statement):
    //   db->execSqlAsyncFuture(
    //       "SELECT item_code, drug_name, manufacturer, shape, "
    //       "color_front, engraving_front, pill_image_url "
    //       "FROM pill_identification WHERE item_code = $1",
    //       item_code);   // ← 바인딩
    //
    // ❌ 금지: "WHERE item_code = '" + item_code + "'" — SQL Injection!
    return std::nullopt;
}

Json::Value
PillIdentificationCache::search_by_keys(const std::string& engraving,
                                        const std::string& shape,
                                        const std::string& color)
{
    // TODO — ✅ 파라미터 바인딩 + LIKE 와일드카드:
    //   db->execSqlAsyncFuture(
    //       "SELECT * FROM pill_identification "
    //       "WHERE engraving_front LIKE $1 AND shape = $2 AND color_front = $3",
    //       "%" + engraving + "%",   // ← 와일드카드는 값에 포함 (sql 결합 X)
    //       shape, color);
    //
    // ⚠ 와일드카드를 SQL 문자열에 직접 넣으면 인젝션 가능 — 반드시 값(파라미터)에 포함
    return Json::Value(Json::arrayValue);
}

} // namespace medibridge::services::pdma
