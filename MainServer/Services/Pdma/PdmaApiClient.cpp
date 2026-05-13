// =====================================================
// PdmaApiClient — 식약처 OpenAPI e약은요 HTTPS 호출
// =====================================================
// 엔드포인트:
//   GET https://apis.data.go.kr/1471000/DrbEasyDrugInfoService/getDrbEasyDrugList
//        ?serviceKey=<key>&itemSeq=<item_code>&type=json&pageNo=1&numOfRows=1
//
// 응답 (예시):
//   {
//     "header": {"resultCode":"00", "resultMsg":"NORMAL SERVICE."},
//     "body": {
//       "items": [{
//         "itemSeq":"199400001",
//         "itemName":"타이레놀정500mg",
//         "efcyQesitm":"이 약은 ...",
//         "useMethodQesitm":"...",
//         "atpnQesitm":"...",   "atpnWarnQesitm":"...",
//         "intrcQesitm":"...",  "seQesitm":"...",
//         "depositMethodQesitm":"..."
//       }],
//       "numOfRows":1, "pageNo":1, "totalCount":1
//     }
//   }
//
// 본 함수는 비동기 (Drogon HttpClient). 응답을 callback 으로 전달.
// 호출자는 DrugOverviewCache 가 일반적 (lazy 캐싱 + INSERT).
// =====================================================
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
    const auto& cfg = Config::instance();
    // 식약처는 HTTPS (TLS) — Drogon HttpClient 가 OpenSSL 사용
    http_client_ = drogon::HttpClient::newHttpClient(cfg.pdma_api_base_url());
    http_client_->setUserAgent("MediBridge/0.1 (PdmaApiClient)");
}

void PdmaApiClient::fetch_drug_overview(const std::string& item_code, Callback callback)
{
    const auto& cfg = Config::instance();

    // API 키 미설정 — production 모드에선 미리 막아야 하지만, 본 함수는
    // POC 단계에서 환경변수가 없어도 호출될 수 있어 안전 가드.
    if (cfg.pdma_service_key().empty()) {
        std::cerr << "[PdmaApiClient] MEDIBRIDGE_PDMA_KEY 미설정 — API 호출 스킵" << std::endl;
        if (callback) callback(Json::Value{}, 503);
        return;
    }

    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/1471000/DrbEasyDrugInfoService/getDrbEasyDrugList");
    req->setParameter("serviceKey", cfg.pdma_service_key());
    req->setParameter("itemSeq",    item_code);
    req->setParameter("type",       "json");
    req->setParameter("pageNo",     "1");
    req->setParameter("numOfRows",  "1");

    http_client_->sendRequest(req,
        [callback = std::move(callback), item_code]
        (drogon::ReqResult result, const drogon::HttpResponsePtr& resp) mutable
        {
            if (result != drogon::ReqResult::Ok || !resp) {
                std::cerr << "[PdmaApiClient] 호출 실패 item_code=" << item_code
                          << " result=" << static_cast<int>(result) << std::endl;
                if (callback) callback(Json::Value{}, 502);
                return;
            }
            const int status = resp->getStatusCode();
            if (status < 200 || status >= 300) {
                std::cerr << "[PdmaApiClient] non-2xx status=" << status
                          << " item_code=" << item_code << std::endl;
                if (callback) callback(Json::Value{}, status);
                return;
            }

            auto json_ptr = resp->getJsonObject();
            if (!json_ptr) {
                std::cerr << "[PdmaApiClient] JSON 파싱 실패 item_code=" << item_code << std::endl;
                if (callback) callback(Json::Value{}, 502);
                return;
            }

            // header.resultCode == "00" 정상. 그 외는 식약처 자체 오류.
            const auto& header = (*json_ptr)["header"];
            const std::string rcode = header.isMember("resultCode")
                ? header["resultCode"].asString() : "";
            if (!rcode.empty() && rcode != "00") {
                std::cerr << "[PdmaApiClient] resultCode=" << rcode
                          << " msg=" << header.get("resultMsg", "").asString() << std::endl;
                if (callback) callback(Json::Value{}, 502);
                return;
            }

            const auto& items = (*json_ptr)["body"]["items"];
            if (!items.isArray() || items.size() == 0) {
                if (callback) callback(Json::Value{}, 404);
                return;
            }

            // 첫 번째 item 만 전달 (numOfRows=1 이라 항상 1개)
            if (callback) callback(items[0], 200);
        });
}

} // namespace medibridge::services::pdma
