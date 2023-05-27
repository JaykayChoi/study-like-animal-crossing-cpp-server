#include "authDb.h"
#include "../sqlLib/connection.h"
#include "../sqlLib/error.h"
#include "../sqlLib/transaction.h"
#include "../util/lacUtil.h"

AuthDb::AuthDb(std::size_t poolSize, const std::string& host, const std::string& user,
    const std::string& passwd, const std::string& db, unsigned int port,
    unsigned long clientflag, const std::map<mysql_option, std::string>& options)
{
    pool_ = new lmysql::ConnPool(
        poolSize, host, user, passwd, db, port, clientflag, options);
}

AuthDb::~AuthDb() { delete pool_; }

AuthDb::LoginResult AuthDb::Login(const std::string& accountId, int curTimeUtc)
{
    LoginResult ret;

    try
    {
        lmysql::Connection conn(*pool_);
        lmysql::Transaction txn(conn);

        try
        {
            std::string query
                = fmt::sprintf("mp_a_account_load('%s')", accountId.c_str());
            std::vector<std::vector<char**>> rows = conn.QueryProcedure(query);

            if (rows.size() > 0 && rows[0].size() > 0)
            {
                ret.bIsNewUser = false;
                lutil::ParseInt(rows[0][0][0], ret.isOnline);

                conn.QueryProcedure(fmt::sprintf(
                    "mp_a_account_update_is_online_and_last_login_time_utc('%s', 1, %d)",
                    accountId.c_str(), curTimeUtc));
            }
            else
            {
                conn.QueryProcedure(fmt::sprintf(
                    "mp_a_account_create('%s', %d)", accountId.c_str(), curTimeUtc));
            }

            // TODO kick 에 필요한 정보 로드.

            txn.Commit();
        }
        catch (const lmysql::Error& err)
        {
            LogError("%s: query error occurred. err: %s", __FUNCTION__, err.what());
            ret.errMsg = err.what();
            txn.Rollback();
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

AuthDb::EnterWorldResult AuthDb::EnterWorld(
    const std::string& accountId, const std::string& worldId)
{
    EnterWorldResult ret;

    try
    {
        lmysql::Connection conn(*pool_);
        lmysql::Transaction txn(conn);

        try
        {
            // TODO account db 에 lastWorldId, lastLobby 추가. lastLobby 은 킥을 위해
            // 필요.

            std::string query;
            std::vector<std::vector<char**>> rows;

            query = fmt::sprintf(
                "mp_a_pub_id_load('%s', '%s')", accountId.c_str(), worldId.c_str());
            rows = conn.QueryProcedure(query);

            std::string pubId;
            int userId;
            if (rows.size() > 0 && rows[0].size() > 0)
            {
                pubId = rows[0][0][0];

                query = fmt::sprintf("mp_a_world_user_load_user_id('%s')", pubId.c_str());
                rows = conn.QueryProcedure(query);
                lutil::ParseInt(rows[0][0][0], userId);
            }
            else
            {
                // Dev 환경에서는 pubId = accountId:worldId
                pubId = accountId;
                pubId.append(":");
                pubId.append(worldId);

                query = fmt::sprintf("mp_a_pub_id_create('%s', '%s', '%s')",
                    accountId.c_str(), pubId.c_str(), worldId.c_str());
                rows = conn.QueryProcedure(query);
                lutil::ParseInt(rows[0][0][0], userId);
            }

            ret.userId = userId;
            ret.pubId = pubId;

            txn.Commit();
        }
        catch (const lmysql::Error& err)
        {
            LogError("%s: query error occurred. err: %s", __FUNCTION__, err.what());
            ret.errMsg = err.what();
            txn.Rollback();
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
