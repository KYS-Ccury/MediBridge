// =====================================================
// PdmaApiClient — 식약처 OpenAPI 외부 호출 (HTTPS)
// =====================================================
// 본 클라이언트는 e약은요 lazy 캐싱 시에만 호출된다.
// 낱알식별·DUR 품목·DUR 성분은 일괄 적재 (Scripts/ImportPdmaData) → 외부 호출 없음.
// =====================================================
#pragma once

#include <drogon/HttpClient.h>
#include <json/json.h>
#include <functional>
#include <string>

namespace medibridge::services::pdma {

class PdmaApiClient
{
public:
    using Callback = std::function<void(const Json::Value& response, int status_code)>;

    static PdmaApiClient& instance();

    /// e약은요 외부 API 호출 (lazy 캐싱 시에만)
    void fetch_drug_overview(const std::string& item_code, Callback callback);

private:
    PdmaApiClient();
    drogon::HttpClientPtr http_client_;
};

} // namespace medibridge::services::pdma
