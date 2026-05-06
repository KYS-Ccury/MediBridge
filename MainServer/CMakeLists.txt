# =====================================================
# MediBridge MainServer (Drogon C++) 빌드 정의
# =====================================================
# Ubuntu 24.04 + Drogon 1.9+ + MariaDB Connector/C 가정.
# 본 파일은 CMake가 메인서버를 빌드하는 방법을 기술.
# =====================================================

cmake_minimum_required(VERSION 3.16)

project(MediBridgeMainServer
    VERSION 0.1.0
    DESCRIPTION "메디브릿지 메인 서버 (FastAPI 대체 — C++ Drogon)"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----- Drogon (HTTP 프레임워크 + ORM) -----
# 설치: Ubuntu에서 sudo apt install drogon-dev (또는 소스 빌드)
find_package(Drogon REQUIRED)

# ----- 추가 의존성 (필요 시 활성화) -----
# bcrypt: 비밀번호 해시 — libbcrypt 또는 OpenSSL EVP_PBKDF2 사용
# JWT:    jwt-cpp 헤더 온리 라이브러리
# find_package(jwt-cpp REQUIRED)

set(SOURCES
    Main.cpp
    Config.cpp
    Routers/Auth.cpp
    Routers/History.cpp
    Routers/Report.cpp
    Routers/Media.cpp
    Routers/Speech.cpp
    Routers/Pill.cpp
    Routers/Monitoring.cpp
    Services/AuthService.cpp
    Services/DurChecker.cpp
    Services/InferenceClient.cpp
    Services/PdmaCacheManager.cpp
    Services/ReportGenerator.cpp
    Database/Connection.cpp
    Threading/WorkerPool.cpp
    Monitoring/ResourceMonitor.cpp
    Monitoring/HealthChecker.cpp
    Monitoring/MetricsExporter.cpp
)

set(HEADERS
    Config.h
    Routers/Auth.h
    Routers/History.h
    Routers/Report.h
    Routers/Media.h
    Routers/Speech.h
    Routers/Pill.h
    Routers/Monitoring.h
    Schemas/AuthSchema.h
    Schemas/HistorySchema.h
    Schemas/ReportSchema.h
    Schemas/MediaSchema.h
    Schemas/SpeechSchema.h
    Schemas/PillSchema.h
    Schemas/HealthSchema.h
    Services/AuthService.h
    Services/DurChecker.h
    Services/InferenceClient.h
    Services/PdmaCacheManager.h
    Services/ReportGenerator.h
    Database/Models.h
    Database/Connection.h
    Threading/WorkerPool.h
    Monitoring/ResourceMonitor.h
    Monitoring/HealthChecker.h
    Monitoring/MetricsExporter.h
)

add_executable(MediBridgeMainServer
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(MediBridgeMainServer PRIVATE
    Drogon::Drogon
    # bcrypt
    # jwt-cpp::jwt-cpp
)

target_include_directories(MediBridgeMainServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

message(STATUS "MediBridgeMainServer — Drogon 버전: ${Drogon_VERSION}")
