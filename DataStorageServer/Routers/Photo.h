// =====================================================
// Photo — PUT / GET /storage/photos/{anon}/{photo}.{ext}
// =====================================================
// 메인서버 발급 토큰(Bearer) 검증 후 디스크 I/O.
// 토큰 op 와 method 가 일치해야 함 (PUT 토큰으로 GET 못함).
// =====================================================
#pragma once

#include <drogon/HttpController.h>

namespace datastorage::routers {

class Photo : public drogon::HttpController<Photo>
{
public:
    METHOD_LIST_BEGIN
    // 경로 파라미터: {anon}/{filename} — filename = <photo_id>.<ext>
    ADD_METHOD_TO(Photo::handle_put,
                  "/storage/photos/{1}/{2}", drogon::Put);
    ADD_METHOD_TO(Photo::handle_get,
                  "/storage/photos/{1}/{2}", drogon::Get);
    METHOD_LIST_END

    void handle_put(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string anon,
                    std::string filename);

    void handle_get(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                    std::string anon,
                    std::string filename);
};

} // namespace datastorage::routers
