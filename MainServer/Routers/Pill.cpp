#include "Pill.h"
#include "../Schemas/PillSchema.h"
#include "../Services/DurChecker.h"
#include "../Services/InferenceClient.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

void Pill::handle_identify(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증
    //   2. IdentifyRequest 파싱
    //   3. user_medication_pool 조회 (식별 범위 좁히기)
    //   4. InferenceClient::detect_and_analyze() — Vision 추론 호출
    //   5. 결과 + user_pool 매칭 → PillCandidate 리스트
    //   6. include_dur_check=true 면 DurChecker::check_combination() 호출
    //   7. IdentifyResponse 200 (LLM 자연어 변환 절대 X)
    //
    // 참고: PillApi.md §1, 표현 톤 정책 (단정 문구 금지)
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void Pill::handle_pool_list(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO: SELECT * FROM user_medication_pool WHERE user_id = ? AND (is_active OR include_inactive)
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void Pill::handle_pool_add(const drogon::HttpRequestPtr& req,
                           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO:
    //   1. PoolAddRequest 파싱
    //   2. item_code 가 pill_identification 에 존재하는지 검증
    //   3. 이미 활성 상태 등록되어 있으면 409 ALREADY_IN_POOL
    //   4. INSERT user_medication_pool
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void Pill::handle_pool_remove(const drogon::HttpRequestPtr& req,
                              std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                              int pool_id)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증 → user_id
    //   2. pool_id 가 본인 데이터인지 확인 (아니면 403)
    //   3. soft delete: UPDATE user_medication_pool SET is_active=FALSE, deactivated_at=NOW()
    //   4. 204 No Content
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void Pill::handle_pool_reset(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. X-Confirm-Reset 헤더 확인 (없으면 400 MISSING_CONFIRM_HEADER)
    //   2. JWT 검증 → user_id
    //   3. 모든 활성 항목 일괄 soft delete
    //   4. 204
    //
    // ⚠ 시스템이 임의로 호출 금지 (FR-B2-03)
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
