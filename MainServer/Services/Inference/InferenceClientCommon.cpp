#include "InferenceClientCommon.h"
#include "../../Config.h"

#include <iostream>

namespace medibridge::services::inference {

InferenceClientCommon::InferenceClientCommon()
{
    const auto& config = Config::instance();
    http_client_ = drogon::HttpClient::newHttpClient(config.inference_server_url());
}

void InferenceClientCommon::post_json(const std::string& path,
                                      const Json::Value& body,
                                      Callback callback)
{
    // TODO (영역 B 분담):
    //   auto req = drogon::HttpRequest::newHttpJsonRequest(body);
    //   req->setPath(path);
    //   req->setMethod(drogon::Post);
    //   http_client_->sendRequest(req, [callback](drogon::ReqResult result, const drogon::HttpResponsePtr& resp) {
    //       if (result == drogon::ReqResult::Ok) {
    //           callback(resp->getJsonObject() ? *resp->getJsonObject() : Json::Value(),
    //                    resp->getStatusCode());
    //       } else {
    //           callback(Json::Value(), 0);  // 네트워크 실패
    //       }
    //   });
    std::cout << "[InferenceClientCommon] post_json TODO: " << path << std::endl;
    if (callback) callback(Json::Value(), 501);
}

void InferenceClientCommon::post_file(const std::string& path,
                                      const std::string& file_path,
                                      const std::string& form_field,
                                      Callback callback)
{
    // TODO: drogon::HttpRequest::newFileUploadRequest({{file_path, form_field}})
    std::cout << "[InferenceClientCommon] post_file TODO: " << path
              << " file=" << file_path << std::endl;
    if (callback) callback(Json::Value(), 501);
}

} // namespace medibridge::services::inference
