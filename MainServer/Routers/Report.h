#pragma once
#include <drogon/HttpController.h>

namespace medibridge::routers {

class Report : public drogon::HttpController<Report>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Report::handle_generate, "/v1/report/generate", drogon::Get);
    METHOD_LIST_END

    void handle_generate(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
