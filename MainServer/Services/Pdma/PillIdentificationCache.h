// =====================================================
// PillIdentificationCache — 식약처 낱알식별 조회 (DB만, 외부 호출 없음)
// =====================================================
// 일괄 적재된 데이터 사용. 운용 중 외부 API 호출 X.
// =====================================================
#pragma once

#include <json/json.h>
#include <optional>
#include <string>

namespace medibridge::services::pdma {

class PillIdentificationCache
{
public:
    /// 품목코드로 낱알식별 정보 조회 (DB만)
    static std::optional<Json::Value>
    get_by_item_code(const std::string& item_code);

    /// 검색 키(각인·모양·색상)로 후보 조회
    static Json::Value
    search_by_keys(const std::string& engraving,
                   const std::string& shape,
                   const std::string& color);
};

} // namespace medibridge::services::pdma
