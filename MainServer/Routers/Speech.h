#pragma once
#include <drogon/HttpController.h>

namespace medibridge::routers {

class Speech : public drogon::HttpController<Speech>
{
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(Speech::handle_utterance, "/v1/speech/utterance", drogon::Post);
    METHOD_LIST_END

    void handle_utterance(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};

} // namespace medibridge::routers
