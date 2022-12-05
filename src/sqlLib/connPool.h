#pragma once

#include "global.h"
#include <windows.h>

namespace lmysql
{
class Connection;

class ConnPool
{
public:
    class PoolEntry
    {
    public:
        PoolEntry(ConnPool* pool, std::size_t poolIndex, const std::string& host,
            const std::string& user, const std::string& passwd, const std::string& db,
            unsigned int port, unsigned long clientflag,
            const std::map<mysql_option, std::string>& options);
        ~PoolEntry();

        void Release();

        bool IsUsed() { return m_bUsed; }

        void SetIsUsed(bool x) { m_bUsed = x; }

        Connection* GetConnection() { return m_Conn; }

    private:
        Connection* m_Conn;

        ConnPool* m_Pool;

        std::size_t m_PoolIndex;

        bool m_bUsed;
    };

    ConnPool(std::size_t poolSize, const std::string& host, const std::string& user,
        const std::string& passwd, const std::string& db, unsigned int port,
        unsigned long clientflag, const std::map<mysql_option, std::string>& options);

    ~ConnPool();

    PoolEntry* GetPoolEntry(int timeout);

    void ReleasePoolEntry(std::size_t index);

private:
    std::vector<PoolEntry*> connections_;

    // Protect connections.
    CRITICAL_SECTION cs_;

    HANDLE handle_;

    DISALLOW_COPY_AND_ASSIGN(ConnPool);

    bool FindFreeConnection(std::size_t& index);
};
}