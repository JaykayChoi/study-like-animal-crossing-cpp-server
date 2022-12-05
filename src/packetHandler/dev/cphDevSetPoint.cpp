#include "cphDevSetPoint.h"
#include "../../clientHandler.h"
#include "json/json.h"

void CphDevSetPoint::Exec(
    PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot)
{
    ClientHandler* client = packetHandler->GetClientHandler();
    if (!bodyRoot.isObject())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::DEV_SET_POINT), "invalid-req-body");
        return;
    }

    std::string cmsId = bodyRoot.get("cmsId", "").asString();
    std::string value = bodyRoot.get("value", "").asString();
    if (cmsId.empty() || value.empty())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::DEV_SET_POINT), "invalid-req-body");
        return;
    }

    // TODO
}