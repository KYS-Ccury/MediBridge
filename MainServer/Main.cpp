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

    // 5. 이벤트 루프 진입 (블록)
    drogon::app().run();

    // 6. 종료 정리
    medibridge::threading::WorkerPool::instance().shutdown();
    std::cout << "[Main] 종료됨" << std::endl;
    return 0;
}
