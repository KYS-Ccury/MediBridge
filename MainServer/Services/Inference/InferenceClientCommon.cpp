// =====================================================
// InferenceClientCommon — Drogon HttpClient 래퍼
// =====================================================
#include "InferenceClientCommon.h"
#include "../../Config.h"

#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>

#include <iostream>

namespace medibridge::services::inference {

InferenceClientCommon::InferenceClientCommon(const std::string& base_url,
                                             int timeout_ms)
    : base_url_(base_url),
      timeout_ms_(timeout_ms)
{
    // Drogon HttpClient 는 Drogon 이벤트 루프에 묶임 (앱 가동 후 생성 안전).
    http_client_ = drogon::HttpClient::newHttpClient(base_url_);
}

void InferenceClientCommon::post_json(const std::string& path,
                                      const Json::Value& body,
                                      Callback callback)
{
    auto req = drogon::HttpRequest::newHttpJsonRequest(body);
    req->setPath(path);
    req->setMethod(drogon::Post);

    http_client_->sendRequest(
        req,
        [cb = std::move(callback)](drogon::ReqResult result,
                                   const drogon::HttpResponsePtr& resp)
        {
            if (result != drogon::ReqResult::Ok || !resp) {
                if (cb) cb(Json::Value(), 0);   // 네트워크/타임아웃
                return;
            }
            const auto json_ptr = resp->getJsonObject();
            const auto json     = json_ptr ? *json_ptr : Json::Value();
            if (cb) cb(json, resp->getStatusCode());
        },
        static_cast<double>(timeout_ms_) / 1000.0);
}

void InferenceClientCommon::post_file(const std::string& path,
                                      const std::string& file_path,
                                      const std::string& form_field,
                                      Callback callback)
{
    drogon::UploadFile upload(file_path, "" /* upload name */, form_field);
    auto req = drogon::HttpRequest::newFileUploadRequest({upload});
    req->setPath(path);
    req->setMethod(drogon::Post);

    http_client_->sendRequest(
        req,
        [cb = std::move(callback)](drogon::ReqResult result,
                                   const drogon::HttpResponsePtr& resp)
        {
            if (result != drogon::ReqResult::Ok || !resp) {
                if (cb) cb(Json::Value(), 0);
                return;
            }
            const auto json_ptr = resp->getJsonObject();
            const auto json     = json_ptr ? *json_ptr : Json::Value();
            if (cb) cb(json, resp->getStatusCode());
        },
        static_cast<double>(timeout_ms_) / 1000.0);
}

} // namespace medibridge::services::inference
