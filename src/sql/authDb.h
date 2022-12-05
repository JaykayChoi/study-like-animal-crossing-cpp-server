#pragma once

#include "../global.h"
#include "../sqlLib/connPool.h"

// TODO db thread 생성.

// TODO rename
// AuthDbManager? AuthDbHelper?
class AuthDb
{
public:
    AuthDb(std::size_t poolSize, const std::string& host, const std::string& user,
        const std::string& passwd, const std::string& db, unsigned int port,
        unsigned long clientflag, const std::map<mysql_option, std::string>& options);

    ~AuthDb();

    struct BaseResult
    {
        std::string errMsg;
    };

    struct LoginResult : public BaseResult
    {
        bool bIsNewUser;
        int isOnline;

        LoginResult()
            : bIsNewUser(true)
            , isOnline(0)
        {
        }
    };
    LoginResult Login(const std::string& accountId, int curTimeUtc);

    struct EnterWorldResult : public BaseResult
    {
        int userId;
        std::string pubId;
    };
    EnterWorldResult EnterWorld(const std::string& accountId, const std::string& worldId);

private:
    lmysql::ConnPool* pool_;
};

/*
Auth table access order

a_accounts
a_pub_ids
a_world_users

*/
