# =====================================================
# MediBridge Client (Qt6 + C++ + QML) 빌드 정의
# =====================================================
# 본 파일은 CMake가 클라이언트 프로젝트를 빌드하는 방법을 기술한다.
# Qt Creator로 이 폴더를 열면 자동 인식된다.
# =====================================================

# CMake 최소 버전 — Qt 6 권장 (3.16 이상 필요)
cmake_minimum_required(VERSION 3.16)

# 프로젝트 이름·언어 정의
project(MediBridgeClient
    VERSION 0.1.0
    DESCRIPTION "메디브릿지 GUI 클라이언트 (Windows 10/11)"
    LANGUAGES CXX
)

# C++17 표준 사용 (Qt 6 권장)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 자동 메타 처리 (시그널·슬롯·QObject)
set(CMAKE_AUTOMOC ON)
# Qt 리소스 파일 자동 처리 (.qrc — 정적 파일 묶기 용도)
set(CMAKE_AUTORCC ON)
# Qt UI 파일 자동 처리 (Qt Designer 사용 시 — 본 프로젝트는 QML이므로 미사용이지만 호환 위해 ON)
set(CMAKE_AUTOUIC ON)

# Qt6 모듈 의존성 — 각 모듈의 역할 주석
find_package(Qt6 REQUIRED COMPONENTS
    Core            # 기본 객체 시스템·이벤트 루프
    Network         # QNetworkAccessManager (MainServer 호출)
    HttpServer      # QHttpServer (PhoneAdapter — 폰 PWA 수신)
    Concurrent      # QtConcurrent::run (워커 스레드 풀 보조)
)

# 소스 파일 목록 (영역별 폴더 구조와 1:1)
set(SOURCES
    Main.cpp
    PhoneAdapter/PhoneServer.cpp

    # MainServerClient — 패턴 B (조립자 + 카테고리별 클라이언트)
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

    # Services — 기능별 분리
    Services/ImageForwarder.cpp
    Services/UtteranceForwarder.cpp

    Monitoring/ResourceMonitor.cpp
    Monitoring/HealthChecker.cpp
)

# 헤더 파일 목록 (AUTOMOC 처리 위해 명시적 등록)
set(HEADERS
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

# 실행 파일 빌드 타깃
add_executable(MediBridgeClient
    ${SOURCES}
    ${HEADERS}
)

# Qt 라이브러리 링크
target_link_libraries(MediBridgeClient PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::HttpServer
    Qt6::Concurrent
)

# include 경로 — 자기 폴더(상대 경로 #include 가능)
target_include_directories(MediBridgeClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Windows 콘솔 앱 명시 (콘솔 로그 보이도록)
if(WIN32)
    set_target_properties(MediBridgeClient PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# 빌드 메시지 (개발자 안내)
message(STATUS "MediBridgeClient — Qt 버전: ${Qt6_VERSION}")
message(STATUS "MediBridgeClient — C++ 표준: C++${CMAKE_CXX_STANDARD}")
