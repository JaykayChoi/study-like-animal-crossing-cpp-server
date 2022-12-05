#pragma once

#include "../packetHandler.h"

namespace Json
{
class Value;
}

class CphTownMove : public IClientPacketHandler
{
public:
    virtual void Exec(
        PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot) override;
};