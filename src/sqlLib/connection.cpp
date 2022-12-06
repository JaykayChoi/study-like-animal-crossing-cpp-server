#include "connection.h"
#include "../util/lacUtil.h"
#include "error.h"

lmysql::Connection::Connection(const std::string& host, const std::string& user,
    const std::string& passwd, const std::string& db, unsigned int port,
    unsigned long clientflag, const std::map<mysql_option, std::string>& options)
    : mysql_(nullptr)
    , bIsFromPool_(false)
    , poolPosition_(0)
    , poolEntry_(nullptr)
{
    Connect(host, user, passwd, db, port, clientflag, options);
}

lmysql::Connection::Connection(ConnPool& pool)
    : bIsFromPool_(true)
    , poolPosition_(0)
{
    poolEntry_ = pool.GetPoolEntry(-1); // No timeout
    mysql_ = poolEntry_->GetConnection()->GetMysql();
}

lmysql::Connection::~Connection()
{
    if (bIsFromPool_)
    {
        poolEntry_->Release();
    }
    else
    {
        CleanUpConnection();
    }
}

std::vector<std::vector<char**>> lmysql::Connection::QueryProcedure(
    const std::string& inQuery)
{
    std::cout << "Cllmysql::Connection::QueryProcedure thread id: "
              << std::this_thread::get_id() << std::endl;

    std::string query = "call ";
    query.append(inQuery);
    return Query(query);
}

std::vector<std::vector<char**>> lmysql::Connection::Query(const std::string& query)
{
    // https://dev.mysql.com/doc/c-api/8.0/en/c-api-multiple-queries.html

    if (!mysql_)
    {
        throw Error("Not connected.");
    }

    std::vector<std::vector<char**>> ret;

    int status = mysql_real_query(
        mysql_, query.c_str(), static_cast<unsigned long>(query.size()));

    if (status != 0)
    {
        throw Error(mysql_error(mysql_), mysql_errno(mysql_));
    }

    do
    {
        MYSQL_RES* mysqlResult = mysql_store_result(mysql_);

        if (mysqlResult)
        {
            std::vector<char**> rowRet;
            processResult(mysqlResult, rowRet);
            ret.push_back(rowRet);
            mysql_free_result(mysqlResult);

            // TODO mysql_fetch_row 필요한지 확인.
        }
        else
        {
            // no result set or error.

            if (mysql_field_count(mysql_) != 0)
            {
                throw Error(mysql_error(mysql_), mysql_errno(mysql_));
            }
            // TODO mysql_affected_rows
        }

        /* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
        if ((status = mysql_next_result(mysql_)) > 0)
        {
            throw Error(mysql_error(mysql_), mysql_errno(mysql_));
        }
    } while (status == 0);

    return ret;
}

void lmysql::Connection::Begin() { Query("BEGIN"); }

void lmysql::Connection::Commit() { Query("COMMIT"); }

void lmysql::Connection::Rollback() { Query("ROLLBACK"); }

void lmysql::Connection::Connect(const std::string& host, const std::string& user,
    const std::string& passwd, const std::string& db, unsigned int port,
    unsigned long clientflag, const std::map<mysql_option, std::string>& options)
{
    mysql_ = mysql_init(nullptr);
    if (!mysql_)
    {
        throw Error("mysql_init is failed.");
    }

    for (const auto& [key, value] : options)
    {
        // TODO: value 필요한 다른 옵션들 처리.
        if (key == MYSQL_OPT_CONNECT_TIMEOUT)
        {
            int number = 0;
            if (!lutil::ParseInt(value, number))
            {
                CleanUpConnection();
                throw Error("Invalid option value.");
            }

            mysql_options(mysql_, key, &number);
        }
        else
        {
            mysql_options(mysql_, key, nullptr);
        }
    }

    if (mysql_real_connect(mysql_, !host.empty() ? host.c_str() : nullptr,
            !user.empty() ? user.c_str() : nullptr,
            !passwd.empty() ? passwd.c_str() : nullptr,
            !db.empty() ? db.c_str() : nullptr, port ? port : 0, nullptr, clientflag)
        == nullptr)
    {
        std::string errMsg = mysql_error(mysql_);
        unsigned int errNum = mysql_errno(mysql_);
        CleanUpConnection();
        throw Error(errMsg, errNum);
    }
}

void lmysql::Connection::CleanUpConnection()
{
    if (mysql_)
    {
        mysql_close(mysql_);
        mysql_library_end();
        mysql_ = nullptr;
    }
}

void lmysql::Connection::processResult(
    MYSQL_RES* mysqlResult, std::vector<char**>& outResult)
{
    MYSQL_ROW row;
    unsigned int numFields;

    numFields = mysql_num_fields(mysqlResult);
    while ((row = mysql_fetch_row(mysqlResult)))
    {
        // unsigned long *lengths;
        // lengths = mysql_fetch_lengths(mysqlResult);
        // for (int i = 0; i < numFields; i++)
        //{
        //	printf("[%.*s] (%d) ", (int)lengths[i],
        //		row[i] ? row[i] : "NULL", (int)lengths[i]);
        // }
        // printf("\n");
        outResult.push_back(row);
    }
}
