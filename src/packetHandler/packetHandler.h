#pragma once

#include "../osLib/byteBuffer.h"
#include "../osLib/criticalSection.h"
#include "crypto.h"

class ClientHandler;
class PacketHandler;
namespace Json
{
class Value;
}

enum class EPacketType : uint32
{
    // Etc

    // Auth
    AUTH_HELLO = 101,
    AUTH_LOGIN = 102,
    AUTH_ENTER_WORLD = 103,

    // Common

    // Town
    TOWN_ENTER = 2001,
    TOWN_LOAD_COMPLETE = 2002,
    TOWN_MOVE_CS = 2003,

    // Dev
    DEV_SET_POINT = 9001,
};

enum class EPayloadFlag : uint8
{
    None = 0,
    Encrypt = 1,
    Compress = 1 << 1,
    Binary = 1 << 2,
};

class IClientPacketHandler
{
public:
    virtual void Exec(
        PacketHandler* packetHandler, int seqNum, const Json::Value& bodyRoot)
        = 0;
};

class PacketHandler
{
public:
    PacketHandler(ClientHandler* client);

    void ReadPayloads(std::string& data);

    void SendJsonPacket(EPacketType packetType, int seqNum, const Json::Value& bodyRoot,
        const EPayloadFlag payloadFlags
        = (EPayloadFlag)((uint8)EPayloadFlag::Encrypt | (uint8)EPayloadFlag::Compress));

    // For client packet handler.
    ClientHandler* GetClientHandler() { return client_; }
    CryptoContext& GetCryptoContext() { return cryptoContext_; }

private:
    ClientHandler* client_;

    CriticalSection csPacket_;

    // 받은 데이터를 위한 버퍼.
    ByteBuffer recvBuffer_;

    std::vector<uint8> recvPayloadBuffer_;
    std::vector<uint8> sendPayloadBuffer_;
    std::vector<uint8> compressBuffer_;
    std::vector<uint8> uncompressBuffer_;
    std::vector<uint8> sendBuffer_;
    std::vector<uint8> decryptBuffer_;
    std::vector<uint8> encryptBuffer_;

    std::map<EPacketType, IClientPacketHandler*> packetHandlers_;

    CryptoContext cryptoContext_;

    void ProcessPayload();

    void ProcessBinaryPacket(ByteBuffer& buffer);

    void ProcessTownMoveBinaryPacket(ByteBuffer& buffer);

    void ProcessJsonPacket(uint8* payloadPtr, int payloadSize);

    bool ProcessPacket(EPacketType packetType, int seqNum, const Json::Value& bodyRoot);

    IClientPacketHandler* GetClientPacketHandler(EPacketType packetType);
};
