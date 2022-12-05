#include "dbConnPool.h"
#include "soci/mysql/soci-mysql.h"
#include "soci/soci.h"

DbConnPool::DbConnPool(std::size_t poolSize, std::string const& connectString)
    : m_PoolSize(poolSize)
{
    soci::backend_factory const& backEnd = *soci::factory_mysql();
    m_Pool = new soci::connection_pool(poolSize);
    for (size_t i = 0; i != poolSize; i++)
    {
        soci::session& sql = m_Pool->at(i);
        sql.open(backEnd, connectString);
    }
}

DbConnPool::~DbConnPool()
{
    // TODO close 필요한지 확인 필요.
    for (size_t i = 0; i != m_PoolSize; i++)
    {
        soci::session& sql = m_Pool->at(i);
        sql.close();
    }

    delete m_Pool;
}
