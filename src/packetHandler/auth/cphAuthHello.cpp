#include "cphAuthHello.h"
#include "../../clientHandler.h"
#include "json/json.h"
#include "../crypto.h"

void CphAuthHello::Exec(
    PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot)
{
    ClientHandler* client = packetHandler->GetClientHandler();
    if (!bodyRoot.isObject())
    {
        client->PacketError(static_cast<uint32>(EPacketType::AUTH_HELLO), "invalid-req-body");
        return;
    }

    std::string publicKey = bodyRoot.get("publicKey", "").asString();
    if (publicKey.empty())
    {
        client->PacketError(static_cast<uint32>(EPacketType::AUTH_HELLO), "invalid-req-body");
        return;
    }

    CryptoContext& cryptoContext = packetHandler->GetCryptoContext();
    if (!cryptoContext.Init())
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::AUTH_HELLO), "failed-to-init-crypro-context");
        return;
    }

    if (!cryptoContext.ComputeSecret(publicKey))
    {
        client->PacketError(
            static_cast<uint32>(EPacketType::AUTH_HELLO), "failed-to-compute-secret");
        return;
    }

    client->OnHello();

    packetHandler->SendJsonPacket(
        EPacketType::AUTH_HELLO, seqNum, cryptoContext.GetMyPublicKey(), EPayloadFlag::None);
}