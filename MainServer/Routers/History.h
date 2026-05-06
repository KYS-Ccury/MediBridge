#pragma once
#include <drogon/HttpController.h>

namespace medibridge::routers {

class History : public drogon::HttpController<History>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(History::handle_record, "/v1/history/record", drogon::Post);
    ADD_METHOD_TO(History::handle_list,   "/v1/history/list",   drogon::Get);
    METHOD_LIST_END

    void handle_record(const drogon::HttpRequestPtr& req,
                       std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void handle_list(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
