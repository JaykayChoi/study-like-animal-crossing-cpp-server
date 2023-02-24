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

void lutil::WriteInt32LEToUInt8Vector(std::vector<uint8>& buf, int value)
{
    buf.push_back(value & 0x000000ff);
    buf.push_back((value & 0x0000ff00) >> 8);
    buf.push_back((value & 0x00ff0000) >> 16);
    buf.push_back((value & 0xff000000) >> 24);
}
