#include "Config.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <stdexcept>

namespace medibridge {

// JWT 시크릿 최소 길이 (HS256 = 256bit = 32바이트)
constexpr size_t JWT_SECRET_MIN_LENGTH = 32;

// 운영 환경 감지용 환경변수 — "production" 시 시크릿 미설정 거부
constexpr const char* MEDIBRIDGE_ENV_VAR = "MEDIBRIDGE_ENV";

Config& Config::instance()
{
    static Config instance;
    return instance;
}

void Config::load_from_file(const std::string& path)
{
    // TODO (영역 B 분담):
    //   1. path 가 존재하면 nlohmann::json 또는 Drogon Json::Value 로 파싱
    //   2. 각 필드를 멤버 변수에 대입
    //   3. 환경변수 우선 적용
    std::cout << "[Config] load_from_file: " << path << " (TODO 구현)" << std::endl;

    // 시크릿 환경변수 적용
    if (const char* pw = std::getenv("MEDIBRIDGE_DB_PASSWORD")) {
        db_password_ = pw;
    }
    if (const char* secret = std::getenv("MEDIBRIDGE_JWT_SECRET")) {
        jwt_secret_ = secret;
    }
    if (const char* pdma = std::getenv("MEDIBRIDGE_PDMA_KEY")) {
        pdma_service_key_ = pdma;
    }

    // TestMode 플래그 — "true"/"1"/"TRUE" 만 활성, 그 외 false
    if (const char* tm = std::getenv("MEDIBRIDGE_TEST_MODE")) {
        std::string v(tm);
        for (auto& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        test_mode_ = (v == "true" || v == "1" || v == "yes" || v == "on");
    }

    // ===== 보안 검증 =====
    const std::string env = std::getenv(MEDIBRIDGE_ENV_VAR)
        ? std::getenv(MEDIBRIDGE_ENV_VAR) : "development";
    const bool is_production = (env == "production");

    // production 환경에서는 TestMode 강제 비활성 (보안 — Docs/TestMode.md §5)
    if (is_production && test_mode_) {
        std::cerr << "[Config] WARN: production 환경에서 MEDIBRIDGE_TEST_MODE 가 설정되어 있어 강제로 false 처리합니다." << std::endl;
        test_mode_ = false;
    }

    if (test_mode_) {
        std::cout << "[Config] MEDIBRIDGE_TEST_MODE = TRUE  → 추론·파일저장 우회, DB seed 사용" << std::endl;
    } else {
        std::cout << "[Config] MEDIBRIDGE_TEST_MODE = FALSE → 운영 모드 (모든 외부 의존성 활성)" << std::endl;
    }

    // 1) JWT 시크릿 검증
    if (jwt_secret_ == "CHANGE_ME_IN_PRODUCTION" || jwt_secret_.empty()) {
        if (is_production) {
            std::cerr << "[Config] FATAL: MEDIBRIDGE_JWT_SECRET 환경변수 미설정 "
                      << "(production 모드에서 기본값 사용 금지)" << std::endl;
            std::abort();
        } else {
            std::cerr << "[Config] WARN: JWT 시크릿이 기본값입니다. "
                      << "production 배포 전 MEDIBRIDGE_JWT_SECRET 환경변수 설정 필수." << std::endl;
        }
    }

    // 2) JWT 시크릿 길이 (HS256 권장 32바이트 이상)
    if (jwt_secret_.length() < JWT_SECRET_MIN_LENGTH) {
        std::cerr << "[Config] WARN: JWT 시크릿 길이 " << jwt_secret_.length()
                  << " < " << JWT_SECRET_MIN_LENGTH
                  << "바이트 — brute force 위험. 32자 이상 권장." << std::endl;
        if (is_production) {
            std::cerr << "[Config] FATAL: production 모드에서 짧은 JWT 시크릿 사용 금지" << std::endl;
            std::abort();
        }
    }

    // 3) DB 비밀번호 검증 (production에서 빈 값 거부)
    if (db_password_.empty() && is_production) {
        std::cerr << "[Config] FATAL: MEDIBRIDGE_DB_PASSWORD 환경변수 미설정 "
                  << "(production 모드 필수)" << std::endl;
        std::abort();
    }
}

uint16_t    Config::server_port()                  const { return server_port_; }
int         Config::drogon_thread_num()            const { return drogon_thread_num_; }
int         Config::worker_pool_size()             const { return worker_pool_size_; }
std::string Config::db_host()                      const { return db_host_; }
uint16_t    Config::db_port()                      const { return db_port_; }
std::string Config::db_user()                      const { return db_user_; }
std::string Config::db_password()                  const { return db_password_; }
std::string Config::db_name()                      const { return db_name_; }
int         Config::db_pool_size()                 const { return db_pool_size_; }
std::string Config::inference_server_url()         const { return inference_server_url_; }
int         Config::inference_request_timeout_ms() const { return inference_request_timeout_ms_; }
std::string Config::pdma_api_base_url()            const { return pdma_api_base_url_; }
std::string Config::pdma_service_key()             const { return pdma_service_key_; }
std::string Config::jwt_secret()                   const { return jwt_secret_; }
int         Config::jwt_expire_seconds()           const { return jwt_expire_seconds_; }
bool        Config::test_mode()                    const { return test_mode_; }

} // namespace medibridge
