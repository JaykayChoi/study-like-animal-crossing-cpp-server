#pragma once

#include "../global.h"
#include "../sqlLib/connPool.h"

class UserDb
{
public:
    UserDb(int shardingSize, std::size_t poolSize, const std::string& host,
        const std::string& user, const std::string& passwd, const std::string& db,
        unsigned int port, unsigned long clientflag,
        const std::map<mysql_option, std::string>& options);

    ~UserDb();

    // TODO AuthDb::BaseResult 와 같이 리팩토링.
    struct BaseResult
    {
        std::string errMsg;
    };

    struct EnterWorldResult : public BaseResult
    {
        int userId;
        std::string name;
        int exp;
        int level;
    };
    EnterWorldResult EnterWorld(int userId, std::string pubId, int curTimeUtc);

private:
    std::vector<lmysql::ConnPool*> pools_;

    lmysql::ConnPool* getDbConnPoolByShardId(int shardId);
    lmysql::ConnPool* getDbConnPoolByUserId(int userId);
};

/*
User table access order

u_users
u_states
u_soft_data

*/
