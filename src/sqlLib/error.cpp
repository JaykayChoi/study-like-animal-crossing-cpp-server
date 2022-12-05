#include "error.h"

lmysql::Error::Error(const std::string& msg, int errNum)
    : std::runtime_error(msg)
    , errNum_(errNum)
{
}
