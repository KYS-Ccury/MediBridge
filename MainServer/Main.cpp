// =====================================================
// MainServer 진입점 (Drogon)
// =====================================================
// Drogon은 HttpController 를 자동 등록하므로 라우팅을 명시적으로 추가하지 않는다.
// 본 main 의 역할:
//   1. 설정(Config) 로드
//   2. DB 연결 풀 초기화 (Drogon 내장)
//   3. WorkerPool 초기화
//   4. drogon::app().run() 으로 이벤트 루프 진입
// =====================================================

#include <drogon/drogon.h>
#include <drogon/orm/Result.h>
#include <drogon/orm/Exception.h>
#include <iostream>

#include "Config.h"
#include "Threading/WorkerPool.h"
#include "Database/Connection.h"

int main(int argc, char *argv[])
{
    // 1. 설정 로드 — config.json 또는 환경변수
    medibridge::Config::instance().load_from_file("config.json");
    const auto& config = medibridge::Config::instance();

    std::cout << "[Main] MediBridge MainServer 시작" << std::endl;
    std::cout << "[Main]   - 포트: " << config.server_port() << std::endl;
    std::cout << "[Main]   - DB: " << config.db_host()
              << ":" << config.db_port()
              << "/" << config.db_name() << std::endl;
    std::cout << "[Main]   - 추론 서버: " << config.inference_server_url() << std::endl;
    std::cout << "[Main]   - TestMode: " << (config.test_mode() ? "ON (DB seed 응답)" : "OFF (운영)") << std::endl;

    // 2. WorkerPool 초기화 (싱글톤)
    medibridge::threading::WorkerPool::instance().init(
        config.worker_pool_size()
    );

    // 3. DB 연결 (Drogon 내장 풀 사용)
    medibridge::database::Connection::instance().init(
        config.db_host(),
        config.db_port(),
        config.db_user(),
        config.db_password(),
        config.db_name(),
        config.db_pool_size()
    );

    // 4. Drogon 앱 설정 (보안 — DoS 방어 옵션 명시)
    drogon::app()
        .addListener("0.0.0.0", config.server_port())
        .setThreadNum(config.drogon_thread_num())
        .setUploadPath("./uploads")
        .setMaxConnectionNum(10000)

        // 보안 — 대용량 요청·연결 폭주 차단
        .setClientMaxBodySize(20 * 1024 * 1024)              // 요청 본문 최대 20MB (이미지 10MB 여유)
        .setClientMaxMemoryBodySize(1 * 1024 * 1024)         // 메모리 처리 최대 1MB (초과 시 디스크)
        .setClientMaxWebSocketMessageSize(1 * 1024 * 1024)   // WebSocket 메시지 최대 1MB
        .setMaxConnectionNumPerIP(50)                         // IP별 동시 연결 50개 제한
        .setIdleConnectionTimeout(60)                         // 유휴 연결 60초 후 종료
        .setKeepaliveRequestsNumber(0);                       // keep-alive 무제한 (LAN 환경)

    // 4-1. 모든 HTTP 요청·응답 단위 로깅 (개발·시연 단계 가시성용)
    //   [REQ] 192.168.x.x  POST /v1/auth/login
    //   [RES] 192.168.x.x  POST /v1/auth/login → 200 (123 bytes)
    drogon::app().registerPreHandlingAdvice(
        [](const drogon::HttpRequestPtr& req,
           drogon::AdviceCallback&&,
           drogon::AdviceChainCallback&& next)
        {
            std::cout << "[REQ] " << req->getPeerAddr().toIpPort()
                      << "  " << req->getMethodString()
                      << " "  << req->getPath()
                      << std::endl;
            std::cout.flush();
            next();
        });

    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr& req,
           const drogon::HttpResponsePtr& resp)
        {
            const int status = resp ? resp->getStatusCode() : 0;
            const auto body = resp ? resp->getBody() : std::string_view{};
            std::cout << "[RES] " << req->getPeerAddr().toIpPort()
                      << "  " << req->getMethodString()
                      << " "  << req->getPath()
                      << "  → " << status
                      << " (" << body.size() << " B)"
                      << std::endl;
            std::cout.flush();
        });

    // =====================================================
    // 4-2. 청소 잡 — photo_storage PENDING 만료 row 를 EXPIRED 로 마킹
    // =====================================================
    // intent → PUT → commit 흐름이 중간에 끊겼을 때 (네트워크 단절·앱 크래시 등)
    // PENDING 상태로 남은 row 를 주기적으로 정리. 토큰은 어차피 exp 검증으로 거부되니
    // 보안에는 영향 없지만, DB 누적 방지 + 운영 가시성 차원.
    //
    // 인터벌: Config::storage_cleanup_interval_seconds (기본 300s, 0 이면 비활성)
    {
        const int interval = config.storage_cleanup_interval_seconds();
        if (interval > 0) {
            drogon::app().getLoop()->runEvery(static_cast<double>(interval),
                []() {
                    auto db = medibridge::database::Connection::instance().client();
                    if (!db) return;
                    db->execSqlAsync(
                        "UPDATE photo_storage SET status='EXPIRED' "
                        " WHERE status='PENDING' AND expires_at < NOW()",
                        [](const drogon::orm::Result& r) {
                            const auto n = r.affectedRows();
                            if (n > 0) {
                                std::cout << "[Cleanup] PENDING → EXPIRED 마킹: "
                                          << n << " 행" << std::endl;
                            }
                        },
                        [](const drogon::orm::DrogonDbException& e) {
                            std::cerr << "[Cleanup] SQL error: "
                                      << e.base().what() << std::endl;
                        });
                });
            std::cout << "[Main]   - 청소 잡 등록 — " << interval
                      << "초 간격 (PENDING expires_at 경과 → EXPIRED)" << std::endl;
        } else {
            std::cout << "[Main]   - 청소 잡 비활성 (interval=0)" << std::endl;
        }
    }

    // 5. 이벤트 루프 진입 (블록)
    drogon::app().run();

    // 6. 종료 정리
    medibridge::threading::WorkerPool::instance().shutdown();
    std::cout << "[Main] 종료됨" << std::endl;
    return 0;
}
