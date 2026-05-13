// =====================================================
// DurQueryEngine — 구현
// =====================================================
// ⚠️ 정책 (절대 준수):
//   1. LLM·RAG 미사용 — 식약처 dur_interaction_cache 그대로 인용
//   2. SQL 파라미터 바인딩 강제 — 문자열 결합 금지 (SQL Injection 방어)
//   3. prohibit_reason 본문 그대로 — action_message 는 정해진 템플릿
//   4. 단정 표현 금지 — "복용 가능합니다" / "복용 불가합니다" 출력 X
//
// 구현 — 페어별 양방향 SELECT (POC 단계):
//   N 개 약 → C(N,2) 페어. 각 페어마다 (A, B) AND (B, A) 두 방향 조회.
//   N=5 면 10 페어 × 2 = 20번 SQL. 평균 응답 < 50ms 예상.
//   부하 증가 시 IN 절 일괄 쿼리로 최적화 가능 (현재는 깔끔성 우선).
// =====================================================
#include "DurQueryEngine.h"
#include "../../Database/Connection.h"
#include "../../Utils/TimeUtil.h"

#include <drogon/orm/Result.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Exception.h>

#include <iostream>
#include <unordered_set>

namespace orm = drogon::orm;

namespace medibridge::services::dur {

namespace {

/// dur 행 → DurDetail (NULL 안전, 단정 표현 금지 템플릿)
schemas::DurDetail row_to_detail(const orm::Row& row)
{
    schemas::DurDetail d;
    d.dur_type            = row["dur_type"].isNull()
        ? std::string{"병용금기"}
        : row["dur_type"].as<std::string>();
    d.drug_a_item_code    = row["base_item_code"].as<std::string>();
    d.drug_a_name         = row["a_name"].isNull()
        ? d.drug_a_item_code
        : row["a_name"].as<std::string>();
    d.drug_b_item_code    = row["target_item_code"].as<std::string>();
    d.drug_b_name         = row["b_name"].isNull()
        ? d.drug_b_item_code
        : row["b_name"].as<std::string>();
    d.prohibit_reason     = row["prohibit_reason"].isNull()
        ? std::string{}
        : row["prohibit_reason"].as<std::string>();
    // ⚠ 단정 표현 금지 — 정해진 안내 템플릿
    d.action_message      = "식약처 안내에 따라 약사·의사와 상담을 권유드립니다.";
    return d;
}

/// (base, target) 단일 페어 조회 — 1방향
std::vector<schemas::DurDetail> query_pair(const std::string& base,
                                           const std::string& target)
{
    auto db = medibridge::database::Connection::instance().client();
    if (!db) return {};

    std::vector<schemas::DurDetail> out;
    try {
        auto r = db->execSqlSync(
            "SELECT dur.dur_type, dur.prohibit_reason, "
            "       dur.base_item_code, dur.target_item_code, "
            "       a.drug_name AS a_name, b.drug_name AS b_name "
            "FROM dur_interaction_cache dur "
            "JOIN pill_identification a ON a.item_code = dur.base_item_code "
            "JOIN pill_identification b ON b.item_code = dur.target_item_code "
            "WHERE dur.base_item_code = ? AND dur.target_item_code = ?",
            base, target);
        for (const auto& row : r) out.push_back(row_to_detail(row));
    } catch (const orm::DrogonDbException& e) {
        std::cerr << "[DurQueryEngine] SQL error: " << e.base().what() << std::endl;
    }
    return out;
}

/// 중복 키 — base+target 사전순 정렬 (양방향 dedup)
std::string pair_key(const std::string& a, const std::string& b)
{
    return (a < b) ? (a + "|" + b) : (b + "|" + a);
}

} // anonymous

schemas::DurCheckResult DurQueryEngine::check_combination(
    const std::vector<std::string>& item_codes)
{
    schemas::DurCheckResult result;
    result.checked_at = utils::current_iso8601_utc();
    result.details    = query_pairwise_risks(item_codes);
    result.result     = result.details.empty() ? "no_risk_found" : "risk_found";
    return result;
}

std::vector<schemas::DurDetail>
DurQueryEngine::query_pairwise_risks(const std::vector<std::string>& item_codes)
{
    std::vector<schemas::DurDetail> out;
    if (item_codes.size() < 2) return out;

    // 양방향 페어 조회. (A,B) 와 (B,A) 둘 다 시도 — dur_interaction_cache 가
    // 어느 방향으로 저장되어 있을지 모르므로. 결과는 dur 페어 키로 dedup.
    std::unordered_set<std::string> seen;
    for (size_t i = 0; i < item_codes.size(); ++i) {
        for (size_t j = i + 1; j < item_codes.size(); ++j) {
            const auto& a = item_codes[i];
            const auto& b = item_codes[j];
            if (a == b) continue;

            for (const auto& d : query_pair(a, b)) {
                const auto k = pair_key(d.drug_a_item_code, d.drug_b_item_code);
                if (seen.insert(k).second) out.push_back(d);
            }
            for (const auto& d : query_pair(b, a)) {
                const auto k = pair_key(d.drug_a_item_code, d.drug_b_item_code);
                if (seen.insert(k).second) out.push_back(d);
            }
        }
    }
    return out;
}

} // namespace medibridge::services::dur
