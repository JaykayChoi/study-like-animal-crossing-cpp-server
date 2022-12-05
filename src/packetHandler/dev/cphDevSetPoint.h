#pragma once

#include "../crypto.h"
#include "../packetHandler.h"

namespace Json
{
class Value;
}

class CphDevSetPoint : public IClientPacketHandler
{
public:
    virtual void Exec(
        PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot) override;
};