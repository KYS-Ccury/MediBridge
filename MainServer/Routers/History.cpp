#include "History.h"
#include "../Schemas/HistorySchema.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

void History::handle_record(const drogon::HttpRequestPtr& req,
                            std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증 → user_id 추출
    //   2. HistoryRecordRequest 파싱·검증
    //   3. intake_datetime 미입력 시 서버 현재 시각 사용
    //   4. medication_intake_logs 에 INSERT (memo, confidence_score, dur_snapshot 포함)
    //   5. HistoryRecordResponse 201
    //
    // 참고: HistoryApi.md §1, DB ERD v2 medication_intake_logs
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

void History::handle_list(const drogon::HttpRequestPtr& req,
                          std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증 → user_id 추출
    //   2. 쿼리 파라미터 (from_date, to_date, item_code, page, page_size, sort_order) 파싱
    //   3. SQL: SELECT ... WHERE user_id = ? AND intake_datetime BETWEEN ? AND ? ...
    //   4. time_slot 자동 분류 (시각 → 아침/점심/저녁/취침)
    //   5. HistoryListResponse 반환
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
