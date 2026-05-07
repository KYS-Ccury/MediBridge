#include "DurQueryEngine.h"
#include "../../Database/Connection.h"
#include "../../Utils/TimeUtil.h"

namespace medibridge::services::dur {

schemas::DurCheckResult DurQueryEngine::check_combination(
    const std::vector<std::string>& item_codes)
{
    schemas::DurCheckResult result;

    // 현재 시각 ISO 8601 — thread-safe 헬퍼 사용 (std::gmtime 미사용)
    result.checked_at = utils::current_iso8601_utc();

    result.details = query_pairwise_risks(item_codes);
    result.result = result.details.empty() ? "no_risk_found" : "risk_found";
    return result;
}

std::vector<schemas::DurDetail>
DurQueryEngine::query_pairwise_risks(const std::vector<std::string>& item_codes)
{
    // TODO (영역 B 분담) — ⚠ SQL 인젝션 방어 절대 준수:
    //
    // ✅ Drogon ORM 사용 (자동 prepared statement):
    //   auto db = drogon::app().getDbClient();
    //   auto future = db->execSqlAsyncFuture(
    //       "SELECT dur_id, base_item_code, target_item_code, dur_type, prohibit_reason "
    //       "FROM dur_interaction_cache "
    //       "WHERE base_item_code = $1 AND target_item_code = $2",
    //       item_a, item_b);     // ← 파라미터 바인딩 (자동 escape)
    //
    // ❌ 절대 금지 — 문자열 결합:
    //   std::string sql = "SELECT ... WHERE base_item_code = '" + item_a + "'";  // SQL Injection!
    //
    // ⚠ prohibit_reason 은 식약처 본문 그대로 (LLM 가공 X)
    //
    // IN (...) 쿼리는 placeholder 갯수만큼 동적 생성:
    //   std::string placeholders;
    //   for (size_t i = 0; i < item_codes.size(); ++i) {
    //       if (i > 0) placeholders += ",";
    //       placeholders += "$" + std::to_string(i + 1);
    //   }
    //   std::string sql = "SELECT ... WHERE base_item_code IN (" + placeholders + ")";
    //   // 그 후 sql 자체엔 사용자 입력 없음, item_codes 는 바인딩으로
    return {};
}

} // namespace medibridge::services::dur
