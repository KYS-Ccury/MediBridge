#include "PdmaCacheManager.h"
#include "../Database/Connection.h"
#include "../Config.h"

namespace medibridge::services {

std::optional<Json::Value>
PdmaCacheManager::get_pill_identification(const std::string& item_code)
{
    // TODO (영역 B 분담):
    //   SELECT * FROM pill_identification WHERE item_code = ?
    //   파라미터 바인딩 필수
    return std::nullopt;
}

std::optional<Json::Value>
PdmaCacheManager::get_drug_overview(const std::string& item_code)
{
    // TODO (영역 B 분담):
    //   1. SELECT * FROM drug_overview WHERE item_code = ?
    //   2. 결과 있으면 그대로 반환
    //   3. 없으면 fetch_drug_overview_from_pdma() 호출 → cache_drug_overview() → 반환
    //   4. 외부 API 실패 시 std::nullopt
    return std::nullopt;
}

std::optional<Json::Value>
PdmaCacheManager::fetch_drug_overview_from_pdma(const std::string& item_code)
{
    // TODO (영역 B 분담):
    //   - drogon::HttpClient 로 식약처 e약은요 API HTTPS 호출
    //   - serviceKey = Config::pdma_service_key()
    //   - URL: Config::pdma_api_base_url() + "/openapi/sample-key/..."
    return std::nullopt;
}

void PdmaCacheManager::cache_drug_overview(const std::string& item_code,
                                           const Json::Value& data)
{
    // TODO: INSERT ... ON DUPLICATE KEY UPDATE drug_overview
}

} // namespace medibridge::services
