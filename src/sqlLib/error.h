#pragma once

#include <stdexcept>
#include <string>

namespace lmysql
{
class Error : public std::runtime_error
{
public:
    Error(const std::string& msg, int errNum = -1);

private:
    int errNum_;
};
}