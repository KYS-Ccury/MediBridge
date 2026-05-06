// =====================================================
// DurQueryEngine — DUR 위험 검출 SQL 조회 엔진
// =====================================================
// ⚠️ LLM·RAG 절대 미사용. 식약처 데이터 그대로 인용.
// =====================================================
#pragma once

#include <vector>
#include <string>
#include "../../Schemas/PillSchema.h"

namespace medibridge::services::dur {

class DurQueryEngine
{
public:
    /// 식별된 약 조합에 대해 페어형 DUR 위험 조회 (병용금기 등)
    static schemas::DurCheckResult check_combination(
        const std::vector<std::string>& item_codes);

private:
    /// MariaDB dur_interaction_cache 페어 조회
    static std::vector<schemas::DurDetail>
    query_pairwise_risks(const std::vector<std::string>& item_codes);
};

} // namespace medibridge::services::dur
