// =====================================================
// MediBridge Client — 진입점 (QGuiApplication + QML)
// =====================================================
// 역할:
//   1. Qt GUI 이벤트 루프 시작
//   2. Backend 모듈 초기화 (Service + Controller)
//   3. Controller·Model 을 QML 컨텍스트에 등록 (rootContext setContextProperty)
//   4. QML 엔진 로드 (qrc:/Frontend/Main.qml)
//   5. PhoneAdapter HTTP 서버 + Monitoring + PhoneLink 시작
// =====================================================

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QLoggingCategory>

// Backend - Service
#include "ApiClient.h"
#include "PhoneServer.h"
#include "ResourceMonitor.h"
#include "HealthChecker.h"
#include "WorkerPool.h"
#include "ImageForwarder.h"
#include "UtteranceForwarder.h"

// Backend - Controller (ViewModel)
#include "AppController.h"
#include "AuthController.h"
#include "PillController.h"
#include "HistoryController.h"
#include "ReportController.h"
#include "PhoneLinkController.h"
#include "VoiceController.h"

// PhoneLink - USB 유선 연동 자동화
#include "AdbDeviceMonitor.h"
#include "AdbReverseManager.h"
#include "PhoneCaptureService.h"

// =====================================================
// 상수
// =====================================================
constexpr quint16 PHONE_ADAPTER_PORT = 8000;
const QString DEFAULT_MAIN_SERVER_URL = QStringLiteral("http://10.10.10.97:8001/");
constexpr int RESOURCE_MONITOR_INTERVAL_MS = 10000;
constexpr int HEALTH_CHECK_INTERVAL_MS = 30000;
constexpr int ADB_DEVICE_POLL_INTERVAL_MS = 5000;

int main(int argc, char *argv[])
{
    // 1. Qt GUI 앱 (QML 사용 위해 QGuiApplication 필요)
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("MediBridgeClient");
    QGuiApplication::setOrganizationName("MediBridge");
    QGuiApplication::setApplicationVersion("0.1.0");

    // 2. Material 스타일 설정 (Q2=A 결정)
    QQuickStyle::setStyle("Material");

    // 3. WorkerPool 초기화
    medibridge::threading::WorkerPool::instance();
    qInfo() << "[Main] WorkerPool 초기화 완료";

    // 4. Backend Service 인스턴스 생성 (메인 스레드 소유)
    medibridge::network::ApiClient api_client(DEFAULT_MAIN_SERVER_URL);
    qInfo() << "[Main] ApiClient 생성 — base_url:" << DEFAULT_MAIN_SERVER_URL;

    medibridge::services::ImageForwarder image_forwarder(&api_client);
    medibridge::services::UtteranceForwarder utterance_forwarder(&api_client);

    // 5. PhoneAdapter HTTP 서버 시작 (폰 PWA 수신)
    medibridge::phone::PhoneServer phone_server(&api_client);
    if (!phone_server.start_server(PHONE_ADAPTER_PORT)) {
        qCritical() << "[Main] PhoneAdapter 서버 시작 실패 — 종료";
        return 1;
    }

    // 6. Monitoring (콘솔 로그만, UI 미연동)
    medibridge::monitoring::ResourceMonitor resource_monitor(RESOURCE_MONITOR_INTERVAL_MS);
    medibridge::monitoring::HealthChecker health_checker(&api_client, HEALTH_CHECK_INTERVAL_MS);
    resource_monitor.start();
    health_checker.start();

    // 7. PhoneLink — USB 유선 연동 자동화 (Q4=C: 자동+수동)
    medibridge::phonelink::AdbDeviceMonitor adb_monitor(ADB_DEVICE_POLL_INTERVAL_MS);
    medibridge::phonelink::AdbReverseManager adb_reverse(PHONE_ADAPTER_PORT);
    medibridge::phonelink::PhoneCaptureService phone_capture_service;   // PC 트리거 화면 캡쳐
    adb_monitor.start();

    // 8. Controller (ViewModel) 인스턴스 생성
    medibridge::controllers::AppController        app_controller;
    medibridge::controllers::AuthController       auth_controller(&api_client);
    medibridge::controllers::PillController       pill_controller(&api_client, &phone_capture_service);
    medibridge::controllers::HistoryController    history_controller(&api_client);
    medibridge::controllers::ReportController     report_controller(&api_client);
    medibridge::controllers::PhoneLinkController  phone_link_controller(&adb_monitor, &adb_reverse);
    medibridge::controllers::VoiceController voice_controller(&utterance_forwarder);

    // 9. QML 엔진 + Controller 컨텍스트 등록
    QQmlApplicationEngine engine;
    auto* root_context = engine.rootContext();
    root_context->setContextProperty("app_controller",        &app_controller);
    root_context->setContextProperty("auth_controller",       &auth_controller);
    root_context->setContextProperty("pill_controller",       &pill_controller);
    root_context->setContextProperty("history_controller",    &history_controller);
    root_context->setContextProperty("report_controller",     &report_controller);
    root_context->setContextProperty("phone_link_controller", &phone_link_controller);
    root_context->setContextProperty("voice_controller",      &voice_controller);

    // 10. QML 엔진 종료 시 앱 종료
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    // 11. 진입 QML 로드
    engine.load(QUrl(QStringLiteral("qrc:/Frontend/Main.qml")));

    qInfo() << "[Main] MediBridge Client 시작됨";
    qInfo() << "[Main]   - PhoneAdapter port:" << PHONE_ADAPTER_PORT;
    qInfo() << "[Main]   - MainServer url:" << DEFAULT_MAIN_SERVER_URL;
    qInfo() << "[Main]   - QML: qrc:/Frontend/Main.qml";

    return app.exec();
}
