#include "DrugOverviewCache.h"
#include "PdmaApiClient.h"
#include "../../Database/Connection.h"

namespace medibridge::services::pdma {

std::optional<Json::Value>
DrugOverviewCache::get_or_fetch(const std::string& item_code)
{
    // TODO (영역 B 분담):
    //   1. get_from_db(item_code) — 있으면 반환
    //   2. 없으면 PdmaApiClient::fetch_drug_overview(item_code) — 외부 API
    //   3. 응답 받으면 cache(item_code, data) — DB 누적
    //   4. 외부 API 실패 시 std::nullopt
    if (auto cached = get_from_db(item_code); cached.has_value()) {
        return cached;
    }
    // TODO: 외부 API 호출은 비동기인데 여기는 동기 시그니처 — Drogon 코루틴 대응 필요
    return std::nullopt;
}

void DrugOverviewCache::cache(const std::string& item_code, const Json::Value& data)
{
    // TODO: INSERT ... ON DUPLICATE KEY UPDATE drug_overview
}

std::optional<Json::Value> DrugOverviewCache::get_from_db(const std::string& item_code)
{
    // TODO: SELECT * FROM drug_overview WHERE item_code = ?
    return std::nullopt;
}

} // namespace medibridge::services::pdma
