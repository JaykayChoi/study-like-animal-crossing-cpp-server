#include "transaction.h"
#include "error.h"

lmysql::Transaction::Transaction(Connection &conn)
    : conn_(conn), bHandled_(false)
{
    conn.Begin();
}

lmysql::Transaction::~Transaction()
{
    if (!bHandled_)
    {
        try
        {
            Rollback();
        }
        catch (...)
        {
        }
    }
}

void lmysql::Transaction::Commit()
{
    if (bHandled_)
    {
        throw Error("Tansaction is handled already.");
    }

    conn_.Commit();
    bHandled_ = true;
}

void lmysql::Transaction::Rollback()
{
    if (bHandled_)
    {
        throw Error("Tansaction is handled already.");
    }

    conn_.Rollback();
    bHandled_ = true;
}
