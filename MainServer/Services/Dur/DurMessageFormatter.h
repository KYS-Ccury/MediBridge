// =====================================================
// DurMessageFormatter — 정해진 템플릿 메시지 생성
// =====================================================
// ⚠️ 단정 표현 금지 ("복용 가능합니다" / "복용 불가합니다" ❌).
// 위험: 식약처 본문 그대로 인용 + "즉시 약사·의사 상담 필요"
// 안전: "DUR 등록 확인되지 않음 + 약사·의사 상담 권유"
// =====================================================
#pragma once

#include <string>
#include "../../Schemas/PillSchema.h"

namespace medibridge::services::dur {

class DurMessageFormatter
{
public:
    /// 위험 케이스 — 식약처 본문 그대로 인용
    static std::string format_risk_message(const schemas::DurDetail& detail);

    /// 안전 케이스 — 단정 X, 상담 권유
    static std::string format_safe_message();

    /// 안전 케이스 (특정 약 페어)
    static std::string format_safe_message_for_pair(const std::string& drug_a_name,
                                                    const std::string& drug_b_name);
};

} // namespace medibridge::services::dur
