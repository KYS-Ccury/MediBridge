#include "DurMessageFormatter.h"

#include <sstream>

namespace medibridge::services::dur {

std::string DurMessageFormatter::format_risk_message(const schemas::DurDetail& d)
{
    // 정해진 템플릿 — 단정 X, 식약처 본문 그대로 인용
    std::ostringstream oss;
    oss << "[" << d.dur_type << "]\n"
        << d.drug_a_name << " + " << d.drug_b_name << "\n"
        << "사유: " << d.prohibit_reason << "\n"
        << "조치: 즉시 약사·의사 상담 필요";
    return oss.str();
}

std::string DurMessageFormatter::format_safe_message()
{
    return "[안전 안내]\n"
           "DUR에 등록 확인되지 않았습니다.\n"
           "안심을 위해 약사·의사 상담을 권유드립니다.";
}

std::string DurMessageFormatter::format_safe_message_for_pair(
    const std::string& drug_a_name, const std::string& drug_b_name)
{
    std::ostringstream oss;
    oss << "[안전 안내]\n"
        << drug_a_name << " + " << drug_b_name << "의 부작용은 "
        << "식약처 DUR에 등록 확인되지 않았습니다.\n"
        << "안심을 위해 약사·의사 상담을 권유드립니다.";
    return oss.str();
}

} // namespace medibridge::services::dur
