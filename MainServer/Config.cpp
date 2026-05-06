#include "Config.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

namespace medibridge {

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
    //   3. 환경변수 우선 적용 (예: getenv("MEDIBRIDGE_DB_PASSWORD"))
    //   4. 시크릿 필드(db_password, jwt_secret)는 반드시 환경변수 강제
    std::cout << "[Config] load_from_file: " << path << " (TODO 구현)" << std::endl;

    // 임시: 환경변수 일부만 처리
    if (const char* pw = std::getenv("MEDIBRIDGE_DB_PASSWORD")) {
        db_password_ = pw;
    }
    if (const char* secret = std::getenv("MEDIBRIDGE_JWT_SECRET")) {
        jwt_secret_ = secret;
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

} // namespace medibridge
