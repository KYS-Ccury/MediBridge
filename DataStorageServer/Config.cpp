#include "Config.h"

#include <cstdlib>
#include <iostream>
#include <filesystem>

namespace datastorage {

namespace fs = std::filesystem;

constexpr size_t SECRET_MIN_LENGTH = 32;

Config& Config::instance()
{
    static Config inst;
    return inst;
}

void Config::load()
{
    if (const char* env = std::getenv("MEDIBRIDGE_ENV"))
        is_production_ = (std::string(env) == "production");

    if (const char* p = std::getenv("DATASTORAGE_PORT")) {
        try { server_port_ = static_cast<uint16_t>(std::stoi(p)); } catch (...) {}
    }
    if (const char* t = std::getenv("DATASTORAGE_THREAD_NUM")) {
        try { thread_num_ = std::stoi(t); } catch (...) {}
    }
    if (const char* r = std::getenv("DATASTORAGE_ROOT")) {
        storage_root_ = r;
    }
    if (const char* s = std::getenv("MEDIBRIDGE_STORAGE_SECRET")) {
        shared_secret_ = s;
    }
    if (const char* m = std::getenv("DATASTORAGE_MAX_BYTES")) {
        try { max_bytes_ = std::stol(m); } catch (...) {}
    }

    std::cout << "[Config] DataStorageServer 설정"          << std::endl;
    std::cout << "  - 포트         : " << server_port_      << std::endl;
    std::cout << "  - storage_root : " << storage_root_     << std::endl;
    std::cout << "  - max_bytes    : " << max_bytes_        << std::endl;
    std::cout << "  - production   : " << (is_production_ ? "YES" : "NO") << std::endl;

    // 시크릿 검증
    if (shared_secret_ == "CHANGE_ME_STORAGE_SECRET" || shared_secret_.empty()) {
        if (is_production_) {
            std::cerr << "[Config] FATAL: MEDIBRIDGE_STORAGE_SECRET 환경변수 미설정 (production)" << std::endl;
            std::abort();
        }
        std::cerr << "[Config] WARN: STORAGE 시크릿이 기본값입니다." << std::endl;
    }
    if (shared_secret_.length() < SECRET_MIN_LENGTH) {
        std::cerr << "[Config] WARN: STORAGE 시크릿 길이 " << shared_secret_.length()
                  << " < " << SECRET_MIN_LENGTH << " 바이트. 32자 이상 권장." << std::endl;
        if (is_production_) std::abort();
    }

    // storage_root 자동 생성 (없으면)
    std::error_code ec;
    if (!fs::exists(storage_root_, ec)) {
        if (fs::create_directories(storage_root_, ec)) {
            std::cout << "[Config] storage_root 디렉터리 생성: " << storage_root_ << std::endl;
        } else {
            std::cerr << "[Config] FATAL: storage_root 생성 실패: "
                      << storage_root_ << " — " << ec.message() << std::endl;
            std::abort();
        }
    }
}

} // namespace datastorage
