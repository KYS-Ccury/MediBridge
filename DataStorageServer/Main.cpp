// =====================================================
// DataStorageServer 진입점
// =====================================================
// 메디브릿지 데이터 보관 PC — Drogon 미니 서버
// 메인 PC 발급 토큰 검증 후 클라/Vision PC 가 PUT/GET 한다.
// =====================================================

#include <drogon/drogon.h>
#include <iostream>

#include "Config.h"

int main(int /*argc*/, char* /*argv*/[])
{
    datastorage::Config::instance().load();
    const auto& cfg = datastorage::Config::instance();

    std::cout << "[Main] DataStorageServer 시작 — port " << cfg.server_port() << std::endl;

    drogon::app()
        .addListener("0.0.0.0", cfg.server_port())
        .setThreadNum(cfg.thread_num())
        // 보안 — 본문 한도. PUT 크기 = 토큰 max(<=10MB). 헤더 여유 2MB 추가.
        .setClientMaxBodySize(cfg.max_bytes() + 2 * 1024 * 1024)
        .setClientMaxMemoryBodySize(2 * 1024 * 1024)   // 2MB 초과 시 디스크 임시 사용
        .setMaxConnectionNum(2000)
        .setMaxConnectionNumPerIP(20)
        .setIdleConnectionTimeout(60)
        .setKeepaliveRequestsNumber(0);

    // 모든 요청/응답 로깅
    drogon::app().registerPreHandlingAdvice(
        [](const drogon::HttpRequestPtr& req,
           drogon::AdviceCallback&&,
           drogon::AdviceChainCallback&& next)
        {
            std::cout << "[REQ] " << req->getPeerAddr().toIpPort()
                      << "  " << req->getMethodString()
                      << " "  << req->getPath() << std::endl;
            std::cout.flush();
            next();
        });
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr& req,
           const drogon::HttpResponsePtr& resp)
        {
            const int status = resp ? resp->getStatusCode() : 0;
            const auto body_sz = resp ? resp->getBody().size() : 0;
            std::cout << "[RES] " << req->getPeerAddr().toIpPort()
                      << "  " << req->getMethodString()
                      << " "  << req->getPath()
                      << "  → " << status
                      << " (" << body_sz << " B)" << std::endl;
            std::cout.flush();
        });

    drogon::app().run();
    std::cout << "[Main] 종료" << std::endl;
    return 0;
}
