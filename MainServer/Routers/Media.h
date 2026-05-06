#pragma once
#include <drogon/HttpController.h>

namespace medibridge::routers {

class Media : public drogon::HttpController<Media>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Media::handle_image_upload, "/v1/media/image", drogon::Post);
    METHOD_LIST_END

    void handle_image_upload(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
