#include "Media.h"
#include "../Services/Inference/InferenceClient.h"

#include <drogon/HttpResponse.h>
#include <drogon/MultiPart.h>

namespace medibridge::routers {

void Media::handle_image_upload(const drogon::HttpRequestPtr& req,
                                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증
    //   2. drogon::MultiPartParser parser; parser.parse(req); → getFiles()
    //   3. 파일 검증 (MIME, 크기 ≤ 10MB)
    //   4. uploads/ 디렉토리에 임시 저장
    //   5. InferenceClient::detect_pills(image_path) 비동기 호출
    //   6. 즉시 202 Accepted 또는 동기 200 OK
    //
    // 참고: MediaApi.md §1, 무거운 인코딩은 WorkerPool 위임
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
