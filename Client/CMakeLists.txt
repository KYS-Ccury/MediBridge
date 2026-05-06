# =====================================================
# MediBridge Client (Qt6 + C++ + QML, MVVM 패턴)
# =====================================================
# Frontend(QML) ↔ Backend(C++) 명확 분리:
#   - Frontend: UI만, 비즈니스 로직 ❌
#   - Backend/Controllers: QML 노출용 ViewModel
#   - Backend/Models: QAbstractListModel 등 데이터 모델
#   - 기존 Service 모듈 (PhoneAdapter, MainServerClient, ...) 그대로 활용
# =====================================================
cmake_minimum_required(VERSION 3.16)

project(MediBridgeClient
    VERSION 0.1.0
    DESCRIPTION "메디브릿지 GUI 클라이언트 (Qt6 + QML + C++)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 모듈
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui                    # QGuiApplication (QML 진입점)
    Qml                    # QQmlApplicationEngine
    Quick                  # QtQuick 프레임워크
    QuickControls2         # Material 스타일
    Network                # QNetworkAccessManager
    HttpServer             # QHttpServer (PhoneAdapter)
    Concurrent             # QtConcurrent
)

set(SOURCES
    Main.cpp

    # Backend - Controllers (ViewModel)
    Backend/Controllers/AppController.cpp
    Backend/Controllers/AuthController.cpp
    Backend/Controllers/PillController.cpp
    Backend/Controllers/HistoryController.cpp
    Backend/Controllers/ReportController.cpp
    Backend/Controllers/PhoneLinkController.cpp
    Backend/Controllers/VoiceController.cpp    # <--- 신규 컨트롤러 추가됨

    # Backend - Models (QAbstractListModel)
    Backend/Models/PillCandidateListModel.cpp
    Backend/Models/HistoryListModel.cpp
    Backend/Models/PoolItemListModel.cpp
    Backend/Models/DurDetailListModel.cpp

    # PhoneLink - USB 유선 연동 자동화
    PhoneLink/AdbDeviceMonitor.cpp
    PhoneLink/AdbReverseManager.cpp
    PhoneLink/PhoneCaptureService.cpp

    # PhoneAdapter (폰 PWA 수신)
    PhoneAdapter/PhoneServer.cpp

    # MainServerClient (조립자 + 카테고리별)
    MainServerClient/ApiClient.cpp
    MainServerClient/ApiClientCommon.cpp
    MainServerClient/AuthApiClient.cpp
    MainServerClient/PillApiClient.cpp
    MainServerClient/HistoryApiClient.cpp
    MainServerClient/ReportApiClient.cpp
    MainServerClient/MediaApiClient.cpp
    MainServerClient/SpeechApiClient.cpp
    MainServerClient/MonitoringApiClient.cpp

    Threading/WorkerPool.cpp

    Services/ImageForwarder.cpp
    Services/UtteranceForwarder.cpp

    Monitoring/ResourceMonitor.cpp
    Monitoring/HealthChecker.cpp
)

set(HEADERS
    Backend/Controllers/AppController.h
    Backend/Controllers/AuthController.h
    Backend/Controllers/PillController.h
    Backend/Controllers/HistoryController.h
    Backend/Controllers/ReportController.h
    Backend/Controllers/PhoneLinkController.h
    Backend/Controllers/VoiceController.h      # <--- 신규 컨트롤러 헤더 추가됨

    Backend/Models/PillCandidateListModel.h
    Backend/Models/HistoryListModel.h
    Backend/Models/PoolItemListModel.h
    Backend/Models/DurDetailListModel.h

    PhoneLink/AdbDeviceMonitor.h
    PhoneLink/AdbReverseManager.h
    PhoneLink/PhoneCaptureService.h

    PhoneAdapter/PhoneServer.h

    MainServerClient/ApiClient.h
    MainServerClient/ApiClientCommon.h
    MainServerClient/AuthApiClient.h
    MainServerClient/PillApiClient.h
    MainServerClient/HistoryApiClient.h
    MainServerClient/ReportApiClient.h
    MainServerClient/MediaApiClient.h
    MainServerClient/SpeechApiClient.h
    MainServerClient/MonitoringApiClient.h

    Threading/WorkerPool.h

    Services/ImageForwarder.h
    Services/UtteranceForwarder.h

    Monitoring/ResourceMonitor.h
    Monitoring/HealthChecker.h
)

# QML 리소스 (Frontend/Resources.qrc 가 .qrc로 묶음)
set(RESOURCES
    Frontend/Resources.qrc
)

add_executable(MediBridgeClient
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(MediBridgeClient PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::HttpServer
    Qt6::Concurrent
)

# 💡 하위 디렉토리 경로들을 Include 경로로 모두 추가
target_include_directories(MediBridgeClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend/Models
    ${CMAKE_CURRENT_SOURCE_DIR}/PhoneLink
    ${CMAKE_CURRENT_SOURCE_DIR}/PhoneAdapter
    ${CMAKE_CURRENT_SOURCE_DIR}/MainServerClient
    ${CMAKE_CURRENT_SOURCE_DIR}/Threading
    ${CMAKE_CURRENT_SOURCE_DIR}/Services
    ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring
)

# Windows: 콘솔 창 표시 (개발 중 로그 확인). 배포 시 FALSE → TRUE 변경
if(WIN32)
    set_target_properties(MediBridgeClient PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

message(STATUS "MediBridgeClient — Qt: ${Qt6_VERSION} / Build: GUI(QML)")