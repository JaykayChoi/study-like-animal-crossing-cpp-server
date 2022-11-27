#include "jsonUtil.h"
#include "json/json.h"

std::string json::Stringify(const Json::Value& root)
{
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

bool json::Parse(
    const std::string& jsonStr, Json::Value& root, std::string* errMsg /*= nullptr*/)
{
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    const char* beginDoc = jsonStr.c_str();
    return reader->parse(beginDoc, beginDoc + jsonStr.size(), &root, errMsg);
}
