// =====================================================
// Config — config.json 파싱 + 환경변수 + 보안 검증
// =====================================================
// 우선순위 (낮음 → 높음):
//   1. 멤버 변수 기본값 (Config.h)
//   2. config.json 파일 (있으면)
//   3. 환경변수 (항상 최우선)
//
// 비밀 정보(db_password / jwt_secret / storage_secret / pdma_key)는
// config.json 에 절대 넣지 말 것. 환경변수 전용.
// 본 구현은 jsoncpp(Drogon 동반) 사용 — nlohmann/json 추가 의존성 없음.
// =====================================================
#include "Config.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cctype>

#include <json/json.h>     // jsoncpp — Drogon 통해 이미 링크됨

namespace medibridge {

constexpr size_t JWT_SECRET_MIN_LENGTH = 32;
constexpr const char* MEDIBRIDGE_ENV_VAR = "MEDIBRIDGE_ENV";

Config& Config::instance()
{
    static Config instance;
    return instance;
}

// =====================================================
// 헬퍼 — JSON 안전 추출 (타입 불일치 시 무시)
// =====================================================
namespace {

const Json::Value& dig(const Json::Value& root, const std::string& path)
{
    static const Json::Value kNull;
    if (root.isNull() || !root.isObject()) return kNull;

    const Json::Value* cur = &root;
    size_t start = 0;
    while (start <= path.size()) {
        const auto dot = path.find('.', start);
        const auto key = path.substr(start, dot == std::string::npos
                                            ? std::string::npos : dot - start);
        if (!cur->isObject() || !cur->isMember(key)) return kNull;
        cur = &(*cur)[key];
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return *cur;
}

template <typename T>
void apply_if(T& target, const Json::Value& v);

template <> void apply_if<std::string>(std::string& target, const Json::Value& v)
{
    if (v.isString()) target = v.asString();
}
template <> void apply_if<int>(int& target, const Json::Value& v)
{
    if (v.isIntegral()) target = v.asInt();
}
template <> void apply_if<long>(long& target, const Json::Value& v)
{
    if (v.isIntegral()) target = static_cast<long>(v.asInt64());
}
template <> void apply_if<uint16_t>(uint16_t& target, const Json::Value& v)
{
    if (v.isIntegral()) {
        const auto x = v.asInt();
        if (x >= 0 && x <= 65535) target = static_cast<uint16_t>(x);
    }
}
template <> void apply_if<bool>(bool& target, const Json::Value& v)
{
    if (v.isBool()) target = v.asBool();
}

bool parse_bool_str(const char* s)
{
    if (!s) return false;
    std::string v(s);
    for (auto& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return v == "true" || v == "1" || v == "yes" || v == "on";
}

} // anonymous

// =====================================================
// 1) config.json 파싱 (선택적 — 없으면 건너뜀)
// =====================================================
void Config::apply_json_file(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cout << "[Config] config.json 없음 (" << path
                  << ") — 환경변수와 기본값만 사용" << std::endl;
        return;
    }

    Json::CharReaderBuilder rb;
    Json::Value root;
    std::string errs;
    if (!Json::parseFromStream(rb, ifs, &root, &errs)) {
        std::cerr << "[Config] config.json 파싱 실패 (" << path << "): "
                  << errs << " — 환경변수와 기본값으로 계속" << std::endl;
        return;
    }

    // server.*
    apply_if(server_port_,        dig(root, "server.port"));
    apply_if(drogon_thread_num_,  dig(root, "server.thread_num"));
    apply_if(worker_pool_size_,   dig(root, "server.worker_pool_size"));

    // db.*  (db.password 는 환경변수 전용 — JSON 적용 X)
    apply_if(db_host_,            dig(root, "db.host"));
    apply_if(db_port_,            dig(root, "db.port"));
    apply_if(db_user_,            dig(root, "db.user"));
    apply_if(db_name_,            dig(root, "db.name"));
    apply_if(db_pool_size_,       dig(root, "db.pool_size"));

    // inference.*
    apply_if(inference_llm_base_,           dig(root, "inference.llm_base"));
    apply_if(inference_vision_base_,        dig(root, "inference.vision_base"));
    apply_if(inference_request_timeout_ms_, dig(root, "inference.request_timeout_ms"));

    // pdma.*  (pdma.service_key 는 환경변수 전용)
    apply_if(pdma_api_base_url_,  dig(root, "pdma.api_base_url"));

    // jwt.*   (jwt.secret 는 환경변수 전용)
    apply_if(jwt_expire_seconds_, dig(root, "jwt.expire_seconds"));

    // storage.*  (storage.secret 은 환경변수 전용)
    apply_if(storage_base_url_,                  dig(root, "storage.base_url"));
    apply_if(storage_token_ttl_seconds_,         dig(root, "storage.token_ttl_seconds"));
    apply_if(storage_max_bytes_,                 dig(root, "storage.max_bytes"));
    apply_if(storage_cleanup_interval_seconds_,  dig(root, "storage.cleanup_interval_seconds"));

    // top-level
    apply_if(test_mode_, dig(root, "test_mode"));

    std::cout << "[Config] config.json 적용 완료 (" << path << ")" << std::endl;
}

// =====================================================
// 2) 환경변수 — JSON 보다 최우선
// =====================================================
void Config::apply_env_vars()
{
    // ---- 비밀 ----
    if (const char* pw = std::getenv("MEDIBRIDGE_DB_PASSWORD")) db_password_ = pw;
    if (const char* secret = std::getenv("MEDIBRIDGE_JWT_SECRET")) jwt_secret_ = secret;
    if (const char* pdma = std::getenv("MEDIBRIDGE_PDMA_KEY")) pdma_service_key_ = pdma;
    if (const char* ss = std::getenv("MEDIBRIDGE_STORAGE_SECRET")) storage_secret_ = ss;

    // ---- 베이스 URL ----
    if (const char* llm = std::getenv("MEDIBRIDGE_INFERENCE_LLM_BASE"))
        inference_llm_base_ = llm;
    if (const char* vis = std::getenv("MEDIBRIDGE_INFERENCE_VISION_BASE"))
        inference_vision_base_ = vis;
    if (const char* sb = std::getenv("MEDIBRIDGE_STORAGE_BASE_URL"))
        storage_base_url_ = sb;

    // ---- 숫자 ----
    if (const char* tt = std::getenv("MEDIBRIDGE_STORAGE_TOKEN_TTL")) {
        try { storage_token_ttl_seconds_ = std::stoi(tt); } catch (...) {}
    }
    if (const char* mb = std::getenv("MEDIBRIDGE_STORAGE_MAX_BYTES")) {
        try { storage_max_bytes_ = std::stol(mb); } catch (...) {}
    }
    if (const char* ci = std::getenv("MEDIBRIDGE_STORAGE_CLEANUP_INTERVAL")) {
        try { storage_cleanup_interval_seconds_ = std::stoi(ci); } catch (...) {}
    }

    // ---- TestMode (true/1/yes/on) ----
    if (const char* tm = std::getenv("MEDIBRIDGE_TEST_MODE")) {
        test_mode_ = parse_bool_str(tm);
    }
}

// =====================================================
// 3) 보안 검증 (production 시 abort)
// =====================================================
void Config::validate()
{
    const char* env_c = std::getenv(MEDIBRIDGE_ENV_VAR);
    const std::string env = env_c ? env_c : "development";
    const bool is_production = (env == "production");

    // production 에서 TestMode 강제 비활성
    if (is_production && test_mode_) {
        std::cerr << "[Config] WARN: production 에서 MEDIBRIDGE_TEST_MODE 강제 false 처리" << std::endl;
        test_mode_ = false;
    }

    std::cout << "[Config] MEDIBRIDGE_TEST_MODE = "
              << (test_mode_ ? "TRUE  → 추론·파일저장 우회, DB seed 사용"
                              : "FALSE → 운영 모드 (모든 외부 의존성 활성)")
              << std::endl;

    auto fatal = [&](const std::string& msg) {
        std::cerr << "[Config] FATAL: " << msg << std::endl;
        std::abort();
    };

    // JWT 시크릿
    if (jwt_secret_ == "CHANGE_ME_IN_PRODUCTION" || jwt_secret_.empty()) {
        if (is_production) fatal("MEDIBRIDGE_JWT_SECRET 미설정 (production)");
        std::cerr << "[Config] WARN: JWT 시크릿 기본값. production 배포 전 MEDIBRIDGE_JWT_SECRET 설정 필수." << std::endl;
    }
    if (jwt_secret_.length() < JWT_SECRET_MIN_LENGTH) {
        std::cerr << "[Config] WARN: JWT 시크릿 길이 " << jwt_secret_.length()
                  << " < " << JWT_SECRET_MIN_LENGTH << " 바이트 — 32자 이상 권장." << std::endl;
        if (is_production) fatal("production 짧은 JWT 시크릿 금지");
    }

    // 보관 PC 시크릿
    if (storage_secret_ == "CHANGE_ME_STORAGE_SECRET" || storage_secret_.empty()) {
        if (is_production) fatal("MEDIBRIDGE_STORAGE_SECRET 미설정 (production)");
        std::cerr << "[Config] WARN: STORAGE 시크릿이 기본값입니다." << std::endl;
    }
    if (storage_secret_.length() < JWT_SECRET_MIN_LENGTH) {
        std::cerr << "[Config] WARN: STORAGE 시크릿 길이 " << storage_secret_.length()
                  << " < " << JWT_SECRET_MIN_LENGTH << " 바이트 — 32자 이상 권장." << std::endl;
        if (is_production) fatal("production 짧은 STORAGE 시크릿 금지");
    }

    // DB
    if (db_password_.empty() && is_production) {
        fatal("MEDIBRIDGE_DB_PASSWORD 미설정 (production)");
    }
}

// =====================================================
// 진입점 — Main.cpp 에서 호출
// =====================================================
void Config::load_from_file(const std::string& path)
{
    apply_json_file(path);
    apply_env_vars();
    validate();
}

// =====================================================
// 접근자
// =====================================================
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
std::string Config::inference_llm_base()           const { return inference_llm_base_; }
std::string Config::inference_vision_base()        const { return inference_vision_base_; }
std::string Config::pdma_api_base_url()            const { return pdma_api_base_url_; }
std::string Config::pdma_service_key()             const { return pdma_service_key_; }
std::string Config::jwt_secret()                   const { return jwt_secret_; }
int         Config::jwt_expire_seconds()           const { return jwt_expire_seconds_; }
std::string Config::storage_base_url()             const { return storage_base_url_; }
std::string Config::storage_secret()               const { return storage_secret_; }
int         Config::storage_token_ttl_seconds()    const { return storage_token_ttl_seconds_; }
long        Config::storage_max_bytes()            const { return storage_max_bytes_; }
int         Config::storage_cleanup_interval_seconds() const { return storage_cleanup_interval_seconds_; }
bool        Config::test_mode()                    const { return test_mode_; }

} // namespace medibridge
