#include "Connection.h"

#include <iostream>

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
    // TODO (영역 B 분담):
    //   db_client_ = drogon::orm::DbClient::newMysqlClient(
    //       "host=" + host + " port=" + std::to_string(port)
    //       + " user=" + user + " password=" + password
    //       + " dbname=" + db_name,
    //       pool_size
    //   );
    std::cout << "[DB] init TODO — " << host << ":" << port
              << "/" << db_name << " (pool=" << pool_size << ")" << std::endl;
}

drogon::orm::DbClientPtr Connection::client()
{
    return db_client_;
}

bool Connection::ping()
{
    // TODO: SELECT 1 으로 헬스체크
    return false;
}

} // namespace medibridge::database
