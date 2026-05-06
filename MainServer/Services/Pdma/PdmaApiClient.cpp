#include "PdmaApiClient.h"
#include "../../Config.h"

#include <iostream>

namespace medibridge::services::pdma {

PdmaApiClient& PdmaApiClient::instance()
{
    static PdmaApiClient instance;
    return instance;
}

PdmaApiClient::PdmaApiClient()
{
    const auto& config = Config::instance();
    http_client_ = drogon::HttpClient::newHttpClient(config.pdma_api_base_url());
}

void PdmaApiClient::fetch_drug_overview(const std::string& item_code, Callback callback)
{
    // TODO (영역 B 분담):
    //   - 식약처 e약은요 API path 구성 (serviceKey, item_code 쿼리)
    //   - HTTPS GET 호출
    //   - XML 또는 JSON 응답 파싱
    //   - callback(json, status)
    //
    // 실패 시 외부 API 장애로 간주, 시스템 동작 유지 (FR-B3-05)
    std::cout << "[PdmaApiClient] fetch_drug_overview TODO: " << item_code << std::endl;
    if (callback) callback(Json::Value(), 501);
}

} // namespace medibridge::services::pdma
