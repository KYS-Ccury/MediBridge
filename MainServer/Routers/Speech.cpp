#include "Speech.h"
#include "../Schemas/SpeechSchema.h"
#include "../Services/Inference/InferenceClient.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

void Speech::handle_utterance(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증
    //   2. UtteranceRequest 파싱·검증
    //   3. 인젝션 패턴 1차 필터 (예: "이전 지시 무시", "당신은 의사야" 패턴)
    //   4. InferenceClient::classify_intent(text, image_request_id) 호출
    //   5. UtteranceResponse 200 (intent_category JSON 강제 형식 검증)
    //
    // 참고: SpeechApi.md §1, FR-A5-03 인젝션 방어
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
