// =====================================================
// MediBridge Client — 진입점
// =====================================================
// 역할:
//   1) Qt 이벤트 루프 시작
//   2) 각 모듈 초기화 + 의존성 주입
//   3) PhoneAdapter HTTP 서버 시작 (폰 PWA 수신)
//   4) Monitoring 모듈 시작 (콘솔 로그)
// =====================================================

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QCommandLineParser>

// 본 프로젝트 헤더 — 영역별 폴더 1:1
#include "MainServerClient/ApiClient.h"
#include "PhoneAdapter/PhoneServer.h"
#include "Monitoring/ResourceMonitor.h"
#include "Monitoring/HealthChecker.h"
#include "Threading/WorkerPool.h"

// =====================================================
// 상수 (UPPER_SNAKE_CASE)
// =====================================================
// 폰 PWA가 접속할 PhoneAdapter 포트.
// adb reverse tcp:8000 tcp:8000 으로 폰 localhost:8000 → PC localhost:8000 매핑.
constexpr quint16 PHONE_ADAPTER_PORT = 8000;

// MainServer 기본 URL (LAN IP는 추후 Config.json 으로 분리 예정).
const QString DEFAULT_MAIN_SERVER_URL = QStringLiteral("http://localhost:8001");

// Monitoring 모듈 폴링 주기 (밀리초).
constexpr int RESOURCE_MONITOR_INTERVAL_MS = 10000;   // 10초마다 자체 자원 측정
constexpr int HEALTH_CHECK_INTERVAL_MS = 30000;       // 30초마다 MainServer /health 폴링

/**
 * @brief 프로그램 진입점.
 *
 * Qt 이벤트 루프(QCoreApplication)를 시작하고 각 모듈을 초기화한다.
 * 각 모듈은 메인 스레드에 머무르며, 무거운 작업은 WorkerPool에 위임한다.
 */
int main(int argc, char *argv[])
{
    // 1. Qt 이벤트 루프 인스턴스 — 시그널·슬롯과 비동기 I/O의 기반
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("MediBridgeClient");
    QCoreApplication::setApplicationVersion("0.1.0");

    // 2. 명령행 옵션 파싱 (선택 — 추후 LAN IP 등 받기 위해 자리만 마련)
    QCommandLineParser parser;
    parser.setApplicationDescription("메디브릿지 GUI 클라이언트");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    // 3. WorkerPool 초기화 (싱글톤 — 첫 호출 시 내부 풀 생성)
    //    무거운 CPU 작업은 어디서든 WorkerPool::instance().submit(...) 으로 위임.
    auto& worker_pool = medibridge::threading::WorkerPool::instance();
    Q_UNUSED(worker_pool);
    qInfo() << "[Main] WorkerPool 초기화 완료";

    // 4. MainServer 호출용 클라이언트 생성 (메인 스레드 소유)
    medibridge::network::ApiClient api_client(DEFAULT_MAIN_SERVER_URL);
    qInfo() << "[Main] ApiClient 생성 — base_url:" << DEFAULT_MAIN_SERVER_URL;

    // 5. PhoneAdapter HTTP 서버 시작 (폰 PWA 수신)
    medibridge::phone::PhoneServer phone_server(&api_client);
    if (!phone_server.start_server(PHONE_ADAPTER_PORT)) {
        qCritical() << "[Main] PhoneAdapter 서버 시작 실패 — 종료";
        return 1;
    }

    // 6. Monitoring 모듈 시작 (콘솔 로그만, UI 미연동)
    medibridge::monitoring::ResourceMonitor resource_monitor(RESOURCE_MONITOR_INTERVAL_MS);
    medibridge::monitoring::HealthChecker health_checker(&api_client, HEALTH_CHECK_INTERVAL_MS);
    resource_monitor.start();
    health_checker.start();

    qInfo() << "[Main] MediBridge Client 시작됨";
    qInfo() << "[Main]   - PhoneAdapter port:" << PHONE_ADAPTER_PORT;
    qInfo() << "[Main]   - MainServer url:" << DEFAULT_MAIN_SERVER_URL;
    qInfo() << "[Main]   - 종료: Ctrl+C";

    // 7. 이벤트 루프 진입 — Ctrl+C 까지 블록
    return app.exec();
}
