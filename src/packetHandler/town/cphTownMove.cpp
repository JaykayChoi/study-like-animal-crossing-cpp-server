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

    int x = bodyRoot.get("x", "").asInt();
    int y = bodyRoot.get("y", "").asInt();
    int z = bodyRoot.get("z", "").asInt();
    int degrees = bodyRoot.get("degrees", "").asInt();
    int speed = bodyRoot.get("speed", "").asInt();

    client->HandleTownMove(x, y, z, degrees, speed);
}