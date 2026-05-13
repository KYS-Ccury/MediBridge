#pragma once
#include <drogon/HttpController.h>

namespace medibridge::routers {

class Media : public drogon::HttpController<Media>
{
public:
    METHOD_LIST_BEGIN
    // ⑤+⑥ 신규 — 정상 사진 흐름 (보관 PC 직접 PUT)
    ADD_METHOD_TO(Media::handle_intent,    "/v1/media/intent",    drogon::Post);
    ADD_METHOD_TO(Media::handle_commit,    "/v1/media/commit",    drogon::Post);
    ADD_METHOD_TO(Media::handle_get_token, "/v1/media/get_token", drogon::Post);  // Vision PC 용
    // 레거시/TestMode — 메인서버 로컬 저장 (보관 PC 미가동 시 fallback)
    ADD_METHOD_TO(Media::handle_image_upload, "/v1/media/image", drogon::Post);
    METHOD_LIST_END

    void handle_intent(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_commit(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_get_token(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_image_upload(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
