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
#include <QProcessEnvironment>

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
#include "CameraStreamController.h"

// Backend - TTS 어댑터 (기획서 §8 어필 포인트, 요구사항 FR-C3-02 / FR-C6)
#include "TtsAdapter.h"

// PhoneLink - USB 유선 연동 자동화
#include "AdbDeviceMonitor.h"
#include "AdbReverseManager.h"
#include "PhoneCaptureService.h"

// =====================================================
// 상수
// =====================================================
constexpr quint16 phone_port_DEFAULT = 8000;
// 폰 측 8000 점유 등으로 충돌 시 MEDIBRIDGE_PHONE_PORT=18000 같이 오버라이드.
// 환경변수로 받으면 PhoneServer + adb reverse + 로그까지 일괄 적용.
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

    // PhoneAdapter 포트 결정 — 환경변수 MEDIBRIDGE_PHONE_PORT 우선
    quint16 phone_port = phone_port_DEFAULT;
    {
        const QString env_val = QProcessEnvironment::systemEnvironment()
                                .value(QStringLiteral("MEDIBRIDGE_PHONE_PORT"));
        if (!env_val.isEmpty()) {
            bool ok = false;
            const int n = env_val.toInt(&ok);
            if (ok && n > 0 && n < 65536) {
                phone_port = static_cast<quint16>(n);
                qInfo() << "[Main] MEDIBRIDGE_PHONE_PORT 적용 →" << phone_port;
            } else {
                qWarning() << "[Main] MEDIBRIDGE_PHONE_PORT 무효 값 무시:" << env_val;
            }
        }
    }

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
    //    utterance_forwarder 주입 — 폰 STT 텍스트 수신 시 GUI VoiceController 도 함께 갱신
    medibridge::phone::PhoneServer phone_server(&api_client, &utterance_forwarder);
    if (!phone_server.start_server(phone_port)) {
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
    medibridge::phonelink::AdbReverseManager adb_reverse(phone_port);
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
    medibridge::controllers::CameraStreamController camera_stream_controller(&phone_capture_service);

    // 8-A. TTS 어댑터 (Windows SAPI 한국어, QSettings 영속)
    medibridge::tts::TtsAdapter tts_adapter;
    qInfo() << "[Main] TtsAdapter 준비 — enabled=" << tts_adapter.enabled();

    // PillController → TTS 자동 발화 연동
    //   식별 결과의 tts_text 가 갱신될 때마다 음성 안내.
    //   백엔드가 "정해진 템플릿" 으로 만든 문구를 그대로 발화 — LLM 자연어 가공 X.
    QObject::connect(&pill_controller,
                     &medibridge::controllers::PillController::tts_text_changed,
                     &tts_adapter,
                     [&pill_controller, &tts_adapter]() {
                         const QString text = pill_controller.tts_text();
                         if (!text.isEmpty()) tts_adapter.speak(text);
                     });

    // QML 의존성을 위해 root_context 에 등록할 준비.

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
    root_context->setContextProperty("camera_stream_controller", &camera_stream_controller);
    root_context->setContextProperty("tts_adapter",           &tts_adapter);

    // 10. QML 엔진 종료 시 앱 종료
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    // 11. 진입 QML 로드
    engine.load(QUrl(QStringLiteral("qrc:/Frontend/Main.qml")));

    // 12. 자동 로그인 시도 (FR-C7-04) — QML 엔진 로드 후에 호출해야
    //     Main.qml 의 Connections.onLogin_succeeded 가 stack.replace 로
    //     HomePage 로 자동 이동 가능.
    auth_controller.restore_session();

    qInfo() << "[Main] MediBridge Client 시작됨";
    qInfo() << "[Main]   - PhoneAdapter port:" << phone_port;
    qInfo() << "[Main]   - MainServer url:" << DEFAULT_MAIN_SERVER_URL;
    qInfo() << "[Main]   - QML: qrc:/Frontend/Main.qml";

    return app.exec();
}
