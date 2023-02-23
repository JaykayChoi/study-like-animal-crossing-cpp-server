#include "connPool.h"
#include "connection.h"
#include "error.h"

/////////////////////////////////////////////////////////////////////////////////////////
// lmysql::ConnPool
lmysql::ConnPool::ConnPool(std::size_t poolSize, const std::string& host,
    const std::string& user, const std::string& passwd, const std::string& db,
    unsigned int port, unsigned long clientflag,
    const std::map<mysql_option, std::string>& options)
{
    if (poolSize == 0)
    {
        throw Error("Invalid pool size.");
    }

    connections_.resize(poolSize);
    for (std::size_t i = 0; i != poolSize; i++)
    {
        connections_[i]
            = new PoolEntry(this, i, host, user, passwd, db, port, clientflag, options);
    }

    InitializeCriticalSection(&cs_);
    HANDLE h = CreateSemaphore(
        NULL, static_cast<LONG>(poolSize), static_cast<LONG>(poolSize), NULL);
    if (h == NULL)
    {
        throw Error("Semaphore creating is failed.");
    }
    handle_ = h;
}

lmysql::ConnPool::~ConnPool()
{
    for (std::size_t i = 0; i != connections_.size(); i++)
    {
        delete connections_[i];
    }

    DeleteCriticalSection(&cs_);
    CloseHandle(handle_);
}

lmysql::ConnPool::PoolEntry* lmysql::ConnPool::GetPoolEntry(int timeout)
{
    DWORD cc = WaitForSingleObject(
        handle_, timeout >= 0 ? static_cast<DWORD>(timeout) : INFINITE);

    if (cc == WAIT_OBJECT_0)
    {
        EnterCriticalSection(&cs_);

        std::size_t index = 0;
        if (!FindFreeConnection(index))
        {
            throw Error("Pool getting is failed.");
        }

        connections_[index]->SetIsUsed(true);

        LeaveCriticalSection(&cs_);

        return connections_[index];
    }
    else if (cc == WAIT_TIMEOUT)
    {
        return nullptr;
    }
    else
    {
        throw Error("Pool getting is failed.");
    }
}

void lmysql::ConnPool::ReleasePoolEntry(std::size_t index)
{
    if (index >= connections_.size())
    {
        throw Error("Invalid pool index.");
    }

    EnterCriticalSection(&cs_);

    if (!connections_[index]->IsUsed())
    {
        LeaveCriticalSection(&cs_);
        throw Error("Cannot release pool entry because already free.");
    }

    connections_[index]->SetIsUsed(false);

    LeaveCriticalSection(&cs_);

    ReleaseSemaphore(handle_, 1, NULL);
}

bool lmysql::ConnPool::FindFreeConnection(std::size_t& index)
{
    for (std::size_t i = 0; i != connections_.size(); i++)
    {
        if (!connections_[i]->IsUsed())
        {
            index = i;
            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// lmysql::ConnPool::Conn
lmysql::ConnPool::PoolEntry::PoolEntry(ConnPool* pool, std::size_t poolIndex,
    const std::string& host, const std::string& user, const std::string& passwd,
    const std::string& db, unsigned int port, unsigned long clientflag,
    const std::map<mysql_option, std::string>& options)
    : m_Conn(nullptr)
    , m_Pool(pool)
    , m_PoolIndex(poolIndex)
    , m_bUsed(false)
{
    m_Conn = new Connection(host, user, passwd, db, port, clientflag, options);
}

lmysql::ConnPool::PoolEntry::~PoolEntry()
{
    if (m_Conn)
    {
        delete m_Conn;
    }
}

void lmysql::ConnPool::PoolEntry::Release() { m_Pool->ReleasePoolEntry(m_PoolIndex); }
