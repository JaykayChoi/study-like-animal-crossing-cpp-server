#include "lacUtil.h"

bool lutil::ParseInt(const std::string str, int& out)
{
    char* tail;
    errno = 0;
    long number = std::strtol(str.c_str(), &tail, 10);
    if (errno != 0 || number < LONG_MIN || number > LONG_MAX || *tail != '\0')
    {
        out = std::numeric_limits<int>::quiet_NaN();
        return false;
    }
    out = (int)number;
    return true;
}
