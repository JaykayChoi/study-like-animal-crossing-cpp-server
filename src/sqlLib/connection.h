#pragma once

#include "../global.h"
#include "connPool.h"
#include <mysql.h>

namespace lmysql
{
class Connection
{
public:
    Connection(const std::string& host, const std::string& user,
        const std::string& passwd, const std::string& db, unsigned int port,
        unsigned long clientflag, const std::map<mysql_option, std::string>& options);

    explicit Connection(ConnPool& pool);

    ~Connection();

    std::vector<std::vector<char**>> QueryProcedure(const std::string& inQuery);

    std::vector<std::vector<char**>> Query(const std::string& query);

    void Begin();
    void Commit();
    void Rollback();

    MYSQL* GetMysql() { return mysql_; }

private:
    MYSQL* mysql_;
    bool bIsFromPool_;
    std::size_t poolPosition_;
    ConnPool::PoolEntry* poolEntry_;

    void Connect(const std::string& host, const std::string& user,
        const std::string& passwd, const std::string& db, unsigned int port,
        unsigned long clientflag, const std::map<mysql_option, std::string>& options);

    void CleanUpConnection();

    void processResult(MYSQL_RES* mysqlResult, std::vector<char**>& outResult);

    DISALLOW_COPY_AND_ASSIGN(Connection);
};
}