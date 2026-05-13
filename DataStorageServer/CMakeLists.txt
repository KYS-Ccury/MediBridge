# =====================================================
# MediBridge DataStorageServer (Drogon C++) 빌드 정의
# =====================================================
# 메인서버와 동일 스택. PUT/GET + 토큰 검증만 담당하는 미니 서버.
# Ubuntu 24.04 + Drogon 1.9+
# =====================================================

cmake_minimum_required(VERSION 3.16)

project(MediBridgeDataStorageServer
    VERSION 0.1.0
    DESCRIPTION "메디브릿지 데이터 보관 PC — 사진 PUT/GET"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)
find_package(OpenSSL REQUIRED)

set(SOURCES
    Main.cpp
    Config.cpp
    Routers/Photo.cpp
    Routers/Monitoring.cpp
    Services/TokenVerifier.cpp
    Services/StorageManager.cpp
)

set(HEADERS
    Config.h
    Routers/Photo.h
    Routers/Monitoring.h
    Services/TokenVerifier.h
    Services/StorageManager.h
)

add_executable(MediBridgeDataStorageServer
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(MediBridgeDataStorageServer PRIVATE
    Drogon::Drogon
    OpenSSL::Crypto
)

target_include_directories(MediBridgeDataStorageServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

message(STATUS "DataStorageServer — Drogon: ${Drogon_VERSION}")
