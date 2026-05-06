#include "Report.h"
#include "../Services/Report/ReportBuilder.h"
#include "../Services/Report/ReportHtmlRenderer.h"
#include "../Services/Report/ReportPdfRenderer.h"
#include "../Threading/WorkerPool.h"

#include <drogon/HttpResponse.h>

namespace medibridge::routers {

void Report::handle_generate(const drogon::HttpRequestPtr& req,
                             std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    // TODO (영역 B 분담):
    //   1. JWT 검증 → user_id
    //   2. 쿼리: from_date, to_date, format (pdf/html/json)
    //   3. 무거운 PDF 렌더링은 WorkerPool::instance().submit(...) 위임
    //   4. format 별 응답 생성 (Content-Type, Content-Disposition)
    //
    // 참고: ReportApi.md, 부작용은 식약처 e약은요 인용 (LLM 변환 X)
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace medibridge::routers
