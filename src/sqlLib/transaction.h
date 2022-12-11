#pragma once

#include "../global.h"
#include "connection.h"

namespace lmysql
{
class Transaction
{
public:
    explicit Transaction(Connection& conn);
    ~Transaction();

    void Commit();
    void Rollback();

private:
    Connection& conn_;
    bool bHandled_;

    DISALLOW_COPY_AND_ASSIGN(Transaction);
};
}