#pragma once

#include "../global.h"

namespace lutil
{
bool ParseInt(const std::string str, int& out);

void WriteInt32LEToUInt8Vector(std::vector<uint8>& buf, int value);
}