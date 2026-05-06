#pragma once
#include <drogon/HttpController.h>

namespace medibridge::routers {

class Pill : public drogon::HttpController<Pill>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Pill::handle_identify,    "/v1/pill/identify",     drogon::Post);
    ADD_METHOD_TO(Pill::handle_pool_list,   "/v1/pill/pool",         drogon::Get);
    ADD_METHOD_TO(Pill::handle_pool_add,    "/v1/pill/pool",         drogon::Post);
    ADD_METHOD_TO(Pill::handle_pool_remove, "/v1/pill/pool/{1}",     drogon::Delete);
    ADD_METHOD_TO(Pill::handle_pool_reset,  "/v1/pill/pool/all",     drogon::Delete);
    METHOD_LIST_END

    void handle_identify(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_pool_list(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_pool_add(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_pool_remove(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                            int pool_id);
    void handle_pool_reset(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
