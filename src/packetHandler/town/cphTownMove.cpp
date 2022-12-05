#include "cphTownMove.h"
#include "../../clientHandler.h"
#include "json/json.h"

void CphTownMove::Exec(
    PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot)
{
    ClientHandler* client = packetHandler->GetClientHandler();
    if (!bodyRoot.isObject())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::TOWN_MOVE_CS), "Invalid-req-body");
        return;
    }

    // TODO

    int x = bodyRoot.get("x", "").asInt();
}