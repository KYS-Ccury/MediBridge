// =====================================================
// Config — 메인서버 전역 설정 (싱글톤)
// =====================================================
// config.json 또는 환경변수에서 로드.
// 모든 설정값은 const 접근자로만 노출.
// =====================================================
#pragma once

#include <string>
#include <cstdint>

namespace medibridge {

class Config
{
public:
    static Config& instance();

    /// config.json 또는 환경변수에서 로드. 둘 다 있으면 환경변수 우선.
    void load_from_file(const std::string& path);

    // ----- HTTP 서버 -----
    uint16_t server_port() const;
    int drogon_thread_num() const;
    int worker_pool_size() const;

    // ----- DB (MariaDB) -----
    std::string db_host() const;
    uint16_t    db_port() const;
    std::string db_user() const;
    std::string db_password() const;
    std::string db_name() const;
    int         db_pool_size() const;

    // ----- 추론 서버 -----
    std::string inference_server_url() const;            // (legacy, 단일 URL)
    int         inference_request_timeout_ms() const;

    // ⭐ v0.3 — 카테고리별 분리 (시스템_연결구조 v2.2)
    std::string inference_llm_base() const;              // LLM PC (10.10.10.120:8002)
    std::string inference_vision_base() const;           // Vision PC (10.10.10.128:8003)

    // ----- 식약처 OpenAPI -----
    std::string pdma_api_base_url() const;
    std::string pdma_service_key() const;

    // ----- JWT -----
    std::string jwt_secret() const;        // HS256 서명 시크릿
    int         jwt_expire_seconds() const;

    // ----- TestMode (Docs/TestMode.md) -----
    /// true 면 추론 서버·데이터 보관 PC 호출을 우회하고 DB seed 데이터로 응답.
    /// production 환경(MEDIBRIDGE_ENV=production)에서는 강제로 false.
    bool test_mode() const;

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    uint16_t    server_port_           = 8001;
    int         drogon_thread_num_     = 0;       // 0 = CPU 코어 수
    int         worker_pool_size_      = 4;

    std::string db_host_               = "127.0.0.1";
    uint16_t    db_port_               = 3306;
    std::string db_user_               = "medibridge_app";
    std::string db_password_           = "";      // config.json 또는 환경변수에서 로드
    std::string db_name_               = "medibridge";
    int         db_pool_size_          = 10;

    std::string inference_server_url_  = "http://127.0.0.1:8002";   // legacy
    int         inference_request_timeout_ms_ = 5000;

    std::string inference_llm_base_    = "http://10.10.10.120:8002";
    std::string inference_vision_base_ = "http://10.10.10.128:8003";

    std::string pdma_api_base_url_     = "https://apis.data.go.kr";
    std::string pdma_service_key_      = "";

    std::string jwt_secret_            = "CHANGE_ME_IN_PRODUCTION";
    int         jwt_expire_seconds_    = 86400;   // 24시간

    bool        test_mode_             = false;   // MEDIBRIDGE_TEST_MODE 로 활성화
};

} // namespace medibridge
