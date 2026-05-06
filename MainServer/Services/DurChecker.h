// =====================================================
// DurChecker — DUR 위험 검출 + 정해진 템플릿 출력 (모듈 1)
// =====================================================
// ⚠️ LLM·RAG 절대 미사용. 식약처 DUR 데이터를 그대로 인용.
// 위반 시 환각·왜곡 리스크 발생.
// =====================================================
#pragma once

#include <vector>
#include <string>
#include "../Schemas/PillSchema.h"

namespace medibridge::services {

class DurChecker
{
public:
    /**
     * @brief 식별된 약 조합에 대해 DUR 위험 검출.
     *
     * @param item_codes 식별된 약 코드 리스트 (사용자 약 풀과 결합 가능)
     * @return 위험 결과 (no_risk_found / risk_found + 식약처 본문 인용)
     */
    static schemas::DurCheckResult check_combination(
        const std::vector<std::string>& item_codes);

    /**
     * @brief 위험 케이스 템플릿 — 정해진 형식만 사용.
     * "약 A + 약 B / 사유: <DUR 본문 그대로> / 조치: 즉시 약사·의사 상담 필요"
     */
    static std::string format_risk_message(const schemas::DurDetail& detail);

    /**
     * @brief 안전 케이스 템플릿 — 단정 X.
     * "DUR에 등록 확인되지 않았습니다. 안심을 위해 약사·의사 상담을 권유드립니다."
     */
    static std::string format_safe_message();

private:
    /// MariaDB dur_interaction_cache 조회 (페어형 카테고리)
    static std::vector<schemas::DurDetail>
    query_pairwise_risks(const std::vector<std::string>& item_codes);
};

} // namespace medibridge::services
