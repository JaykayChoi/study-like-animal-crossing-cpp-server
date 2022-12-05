#include "userDb.h"
#include "../sqlLib/connection.h"
#include "../sqlLib/error.h"
#include "../sqlLib/transaction.h"
#include "../util/lacUtil.h"

UserDb::UserDb(int shardingSize, std::size_t poolSize, const std::string& host,
    const std::string& user, const std::string& passwd, const std::string& db,
    unsigned int port, unsigned long clientflag,
    const std::map<mysql_option, std::string>& options)
{
    for (int i = 0; i < shardingSize; i++)
    {
        std::string shardedDbName = fmt::sprintf("%s_%02d", db, i);
        pools_.push_back(new lmysql::ConnPool(
            poolSize, host, user, passwd, shardedDbName, port, clientflag, options));
    }
}

UserDb::~UserDb()
{
    for (int i = 0; i < pools_.size(); i++)
    {
        delete pools_[i];
    }
    pools_.clear();
}

UserDb::EnterWorldResult UserDb::EnterWorld(int userId, std::string pubId, int curTimeUtc)
{
    EnterWorldResult ret;
    ret.userId = userId;

    try
    {
        lmysql::Connection conn(*getDbConnPoolByUserId(userId));
        lmysql::Transaction txn(conn);

        try
        {
            std::string query;
            std::vector<std::vector<char**>> rows;

            query = fmt::sprintf("mp_u_user_load_id(%d)", userId);
            rows = conn.QueryProcedure(query);

            if (rows.size() > 0 && rows[0].size() > 0)
            {
                query = fmt::sprintf(
                    "mp_u_state_update_last_login_time_utc_and_is_online(%d, %d, 1)",
                    userId, curTimeUtc);
                conn.QueryProcedure(query);
            }
            else
            {
                // New user.
                query = fmt::sprintf(
                    "mp_u_user_create(%d, '%s', %d)", userId, pubId, curTimeUtc);
                conn.QueryProcedure(query);
            }

            txn.Commit();
        }
        catch (const lmysql::Error& err)
        {
            LogError("%s: query error occurred. err: %s", __FUNCTION__, err.what());
            ret.errMsg = err.what();
            txn.Rollback();
            return ret;
        }
    }
    catch (const lmysql::Error& err)
    {
        LogError(
            "%s: error occurred during connecting db. err: %s", __FUNCTION__, err.what());
        ret.errMsg = err.what();
        return ret;
    }

    try
    {
        lmysql::Connection conn(*getDbConnPoolByUserId(userId));

        try
        {
            std::string query = fmt::sprintf("mp_u_user_load(%d)", userId);
            std::vector<std::vector<char**>> rows = conn.QueryProcedure(query);

            if (rows[0][0][0] != NULL)
            {
                ret.name = rows[0][0][0];
            }
            lutil::ParseInt(rows[0][0][1], ret.exp);
            lutil::ParseInt(rows[0][0][2], ret.level);
        }
        catch (const lmysql::Error& err)
        {
            LogError("%s: query error occurred. err: %s", __FUNCTION__, err.what());
            ret.errMsg = err.what();
        }
    }
    catch (const lmysql::Error& err)
    {
        LogError(
            "%s: error occurred during connecting db. err: %s", __FUNCTION__, err.what());
        ret.errMsg = err.what();
    }

    return ret;
}

lmysql::ConnPool* UserDb::getDbConnPoolByShardId(int shardId)
{
    ASSERT(pools_[shardId]);
    return pools_[shardId];
}

lmysql::ConnPool* UserDb::getDbConnPoolByUserId(int userId)
{
    return getDbConnPoolByShardId(userId % pools_.size());
}
