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
    # Services — 패턴 B (카테고리별 클래스 분리, 하위 폴더로 그룹)
    Services/Auth/PasswordHasher.cpp
    Services/Auth/JwtIssuer.cpp
    Services/Auth/UserManager.cpp
    Services/Auth/Authenticator.cpp
    Services/Dur/DurQueryEngine.cpp
    Services/Dur/DurMessageFormatter.cpp
    Services/Inference/InferenceClientCommon.cpp
    Services/Inference/VisionInferenceClient.cpp
    Services/Inference/IntentInferenceClient.cpp
    Services/Inference/OnboardingInferenceClient.cpp
    Services/Inference/SummaryInferenceClient.cpp
    Services/Inference/InferenceClient.cpp
    Services/Pdma/PillIdentificationCache.cpp
    Services/Pdma/DrugOverviewCache.cpp
    Services/Pdma/PdmaApiClient.cpp
    Services/Report/ReportBuilder.cpp
    Services/Report/ReportHtmlRenderer.cpp
    Services/Report/ReportPdfRenderer.cpp
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
    Services/Auth/PasswordHasher.h
    Services/Auth/JwtIssuer.h
    Services/Auth/UserManager.h
    Services/Auth/Authenticator.h
    Services/Dur/DurQueryEngine.h
    Services/Dur/DurMessageFormatter.h
    Services/Inference/InferenceClientCommon.h
    Services/Inference/VisionInferenceClient.h
    Services/Inference/IntentInferenceClient.h
    Services/Inference/OnboardingInferenceClient.h
    Services/Inference/SummaryInferenceClient.h
    Services/Inference/InferenceClient.h
    Services/Pdma/PillIdentificationCache.h
    Services/Pdma/DrugOverviewCache.h
    Services/Pdma/PdmaApiClient.h
    Services/Report/ReportBuilder.h
    Services/Report/ReportHtmlRenderer.h
    Services/Report/ReportPdfRenderer.h
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
