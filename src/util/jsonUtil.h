#pragma once

#include "../global.h"

namespace Json
{
class Value;
}

namespace json
{
std::string Stringify(const Json::Value& root);

bool Parse(const std::string& jsonStr, Json::Value& root, std::string* errMsg = nullptr);
}