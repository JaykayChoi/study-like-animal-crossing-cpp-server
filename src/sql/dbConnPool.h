#pragma once

// TODO remove

#include "../global.h"
#include "soci/connection-pool.h"

// http://soci.sourceforge.net/doc/master/multithreading/
// http://soci.sourceforge.net/doc/master/api/client/

class DbConnPool
{
public:
    DbConnPool(std::size_t poolSize, std::string const& connectString);
    ~DbConnPool();

    soci::connection_pool* getPool() { return m_Pool; }

private:
    soci::connection_pool* m_Pool;

    std::size_t m_PoolSize;
};
