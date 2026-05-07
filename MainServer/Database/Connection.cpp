#include "Connection.h"

#include <iostream>
#include <chrono>

namespace medibridge::database {

Connection& Connection::instance()
{
    static Connection instance;
    return instance;
}

void Connection::init(const std::string& host,
                      uint16_t port,
                      const std::string& user,
                      const std::string& password,
                      const std::string& db_name,
                      int pool_size)
{
    // Drogon DbClient (MariaDB/MySQL) 풀 생성.
    // 모든 쿼리는 파라미터 바인딩($1,$2,…) 으로 작성 → SQL 인젝션 방어.
    const std::string conn = "host=" + host
                           + " port=" + std::to_string(port)
                           + " user=" + user
                           + " password=" + password
                           + " dbname=" + db_name;
    try {
        db_client_ = drogon::orm::DbClient::newMysqlClient(conn, pool_size);
        std::cout << "[DB] connected — " << host << ":" << port
                  << "/" << db_name << " (pool=" << pool_size << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] FATAL — DbClient 생성 실패: " << e.what() << std::endl;
        db_client_.reset();
    }
}

drogon::orm::DbClientPtr Connection::client()
{
    return db_client_;
}

bool Connection::ping()
{
    if (!db_client_) return false;
    try {
        // SELECT 1 동기 호출 — 헬스체크 한정. 일반 쿼리는 비동기 사용.
        auto result = db_client_->execSqlSync("SELECT 1");
        return !result.empty();
    } catch (const std::exception& e) {
        std::cerr << "[DB] ping 실패: " << e.what() << std::endl;
        return false;
    }
}

} // namespace medibridge::database
