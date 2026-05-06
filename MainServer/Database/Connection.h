// =====================================================
// Connection — DB 연결 풀 관리 (싱글톤)
// =====================================================
// Drogon DbClient 풀 사용. 파라미터 바인딩 SQL 인젝션 방어.
// =====================================================
#pragma once

#include <string>
#include <cstdint>
#include <drogon/orm/DbClient.h>

namespace medibridge::database {

class Connection
{
public:
    static Connection& instance();

    /// DB 풀 초기화 (Drogon DbClient::newMysqlClient)
    void init(const std::string& host,
              uint16_t port,
              const std::string& user,
              const std::string& password,
              const std::string& db_name,
              int pool_size);

    /// Drogon DB 클라이언트 (파라미터 바인딩 쿼리에 사용)
    drogon::orm::DbClientPtr client();

    /// 연결 점검 (헬스체크용 SELECT 1)
    bool ping();

private:
    Connection() = default;
    ~Connection() = default;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    drogon::orm::DbClientPtr db_client_;
};

} // namespace medibridge::database
