#pragma once
#include <drogon/HttpController.h>

namespace datastorage::routers {

class Monitoring : public drogon::HttpController<Monitoring>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Monitoring::handle_health, "/health", drogon::Get);
    METHOD_LIST_END

    void handle_health(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace datastorage::routers
