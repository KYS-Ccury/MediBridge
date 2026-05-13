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

    // ----- 데이터 보관 PC (Storage) — ⑤+⑥ 통신 -----
    /// 보관 PC 베이스 URL — 클라/Vision PC 가 직접 PUT/GET (e.g., http://10.10.10.122:8004)
    std::string storage_base_url() const;
    /// 메인 ↔ 보관 PC 공유 시크릿 (PUT/GET 토큰 HMAC-SHA256). JWT 시크릿과 별개.
    std::string storage_secret() const;
    /// PUT/GET 토큰 TTL (기본 300초 = 5분)
    int         storage_token_ttl_seconds() const;
    /// 업로드 허용 최대 바이트 (기본 10MB)
    long        storage_max_bytes() const;
    /// PENDING 청소 잡 인터벌 (초). 0 = 비활성. 기본 300 (5분).
    int         storage_cleanup_interval_seconds() const;

    // ----- TestMode (Docs/TestMode.md) -----
    /// true 면 추론 서버·데이터 보관 PC 호출을 우회하고 DB seed 데이터로 응답.
    /// production 환경(MEDIBRIDGE_ENV=production)에서는 강제로 false.
    bool test_mode() const;

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    /// load_from_file 단계별 헬퍼 (우선순위: JSON < 환경변수 < 검증)
    void apply_json_file(const std::string& path);
    void apply_env_vars();
    void validate();

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

    // ----- 데이터 보관 PC -----
    std::string storage_base_url_         = "http://10.10.10.122:8004";
    std::string storage_secret_           = "CHANGE_ME_STORAGE_SECRET";
    int         storage_token_ttl_seconds_ = 300;        // 5분
    long        storage_max_bytes_         = 10L * 1024L * 1024L;  // 10MB
    int         storage_cleanup_interval_seconds_ = 300; // 5분 — PENDING → EXPIRED 청소

    bool        test_mode_             = false;   // MEDIBRIDGE_TEST_MODE 로 활성화
};

} // namespace medibridge
