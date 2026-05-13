// =====================================================
// DataStorageServer Config — 데이터 보관 PC 설정 (싱글톤)
// =====================================================
// 환경변수에서 로드. 기본값은 개발/시연용.
// =====================================================
#pragma once

#include <string>
#include <cstdint>

namespace datastorage {

class Config
{
public:
    static Config& instance();

    /// 환경변수에서 로드 (호출 1회).
    void load();

    // ----- HTTP 서버 -----
    uint16_t server_port() const   { return server_port_; }
    int      thread_num()  const   { return thread_num_; }

    // ----- 스토리지 -----
    /// 파일 저장 루트 (예: /var/lib/medibridge_storage)
    /// 내부 구조: <root>/<anonymous_id>/<photo_id>.<ext>
    std::string storage_root() const { return storage_root_; }

    /// 메인서버 ↔ 보관 PC 공유 시크릿 (HMAC-SHA256, JWT 와 별개)
    std::string shared_secret() const { return shared_secret_; }

    /// 업로드 허용 최대 바이트 (서버 측 강제 — 토큰의 max 와 별개로 한 번 더 검증)
    long max_bytes() const { return max_bytes_; }

    /// 운영 환경 — production 시 시크릿 미설정·기본값 거부
    bool is_production() const { return is_production_; }

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    uint16_t    server_port_   = 8004;
    int         thread_num_    = 0;                                    // 0 = CPU 코어 수
    std::string storage_root_  = "/var/lib/medibridge_storage";
    std::string shared_secret_ = "CHANGE_ME_STORAGE_SECRET";
    long        max_bytes_     = 10L * 1024L * 1024L;                 // 10MB
    bool        is_production_ = false;
};

} // namespace datastorage
