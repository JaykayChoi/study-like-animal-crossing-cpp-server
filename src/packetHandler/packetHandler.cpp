#include "packetHandler.h"
#include "../clientHandler.h"
#include "../middleware/logger.h"
#include "../util/jsonUtil.h"
#include "auth/cphAuthEnterWorld.h"
#include "auth/cphAuthHello.h"
#include "auth/cphAuthLogin.h"
#include "dev/cphDevSetPoint.h"
#include "town/cphTownMove.h"
#include "zlib.h"
#include "json/json.h"
#include <openssl/opensslconf.h>

const static uint32 HEADER_LEN = 4;
const static uint32 TEMP_BUF_SIZE = 8192;

PacketHandler::PacketHandler(ClientHandler* client)
    : client_(client)
    , recvBuffer_(32 * 1024)
{
}

void PacketHandler::ReadPayloads(std::string& data)
{
    // Write the received data to recvBuffer_
    if (!recvBuffer_.Write(data.data(), data.size()))
    {
        // Queue 에 너무 많은 데이터가 들어온 경우.
        client_->PacketBufferFull();
        return;
    }

    uint8 firstByte, secondByte, payloadFlags, zero;
    if (!recvBuffer_.ReadUInt8(firstByte) || !recvBuffer_.ReadUInt8(secondByte)
        || !recvBuffer_.ReadUInt8(payloadFlags) || !recvBuffer_.ReadUInt8(zero))
    {
        // Not enough data.
        recvBuffer_.ResetRead();
        return;
    }

    recvBuffer_.ResetRead();

    int payloadSize = 0;
    payloadSize |= (firstByte & 0xFF) << 8;
    payloadSize |= secondByte & 0xFF;

    // 패킷 길이에 비해 바이트가 충분하지 않는 경우 기다린다.
    if (!recvBuffer_.CanReadBytes(payloadSize))
    {
        return;
    }

    ProcessPayload();
}

void PacketHandler::SendJsonPacket(EPacketType packetType, int seqNum,
    const Json::Value& bodyRoot, const EPayloadFlag payloadFlags)
{
    Json::Value root;
    root["type"] = static_cast<int>(packetType);
    root["seqNum"] = seqNum;
    root["body"] = json::Stringify(bodyRoot);

    std::string packetStr = json::Stringify(root);
    Log("Transmit packet. Packet: %s", packetStr.c_str());

    sendPayloadBuffer_.assign(packetStr.begin(), packetStr.end());

    uint8* payloadPtr = sendPayloadBuffer_.data();
    int payloadSize = sendPayloadBuffer_.size();
    int uncompressedSize = payloadSize;

    // Compress.
    if ((uint8)payloadFlags & (uint8)EPayloadFlag::Compress)
    {
        int compressedSize = TEMP_BUF_SIZE;
        compressBuffer_.clear();
        compressBuffer_.reserve(compressedSize);
        bool zerr = compress(compressBuffer_.data(), &(uLong)compressedSize, payloadPtr,
            (uLong)payloadSize);

        if (zerr != Z_OK)
        {
            LogError("Failed to compress!");
            return;
        }

        // Log("ZIP Before (%d) Compressed (%d)",
        //	sendPayloadBuffer_.size(), compressedSize);

        payloadPtr = compressBuffer_.data();
        payloadSize = compressedSize;
    }

    // Encrypt.
    if ((uint8)payloadFlags & (uint8)EPayloadFlag::Encrypt)
    {
        int encryptedSize;
        if (!cryptoContext_.Encrypt(
                payloadPtr, payloadSize, encryptBuffer_, encryptedSize))
        {
            LogError("Failed to encrypt.");
            return;
        }

        payloadPtr = encryptBuffer_.data();
        payloadSize = encryptedSize;
    }

    // If compressed, add uncompressed size of 4.
    int packetSize = payloadSize;
    if ((uint8)payloadFlags & (uint8)EPayloadFlag::Compress)
    {
        packetSize += sizeof(uncompressedSize);
    }

    sendBuffer_.clear();
    sendBuffer_.reserve(packetSize + HEADER_LEN);

    uint8 firstByte = (uint8)((packetSize & 0xFF00) >> 8);
    uint8 secondByte = (uint8)(packetSize & 0x00FF);

    sendBuffer_.push_back(firstByte);
    sendBuffer_.push_back(secondByte);
    sendBuffer_.push_back((uint8)payloadFlags);
    sendBuffer_.push_back(0);

    // 클라의 UncompressBuf 사이즈를 최적화 하기 위해 uncompressed size 정보를 넣어줌.
    if ((uint8)payloadFlags & (uint8)EPayloadFlag::Compress)
    {
        // Write uncompressed size. (little endian)
        sendBuffer_.push_back(uncompressedSize & 0x000000ff);
        sendBuffer_.push_back((uncompressedSize & 0x0000ff00) >> 8);
        sendBuffer_.push_back((uncompressedSize & 0x00ff0000) >> 16);
        sendBuffer_.push_back((uncompressedSize & 0xff000000) >> 24);
    }

    sendBuffer_.insert(std::end(sendBuffer_), payloadPtr, payloadPtr + payloadSize);

    client_->SendData(sendBuffer_.data(), sendBuffer_.size());
}

void PacketHandler::ProcessPayload()
{
    for (;;)
    {
        uint8 firstByte, secondByte, payloadFlags, zero;
        if (!recvBuffer_.ReadUInt8(firstByte) || !recvBuffer_.ReadUInt8(secondByte)
            || !recvBuffer_.ReadUInt8(payloadFlags) || !recvBuffer_.ReadUInt8(zero))
        {
            // Not enough data.
            recvBuffer_.ResetRead();
            break;
        }

        int packetSize = 0;
        packetSize |= (firstByte & 0xFF) << 8;
        packetSize |= secondByte & 0xFF;

        if (!recvBuffer_.CanReadBytes(packetSize))
        {
            // The full packet hasn't been received yet
            recvBuffer_.ResetRead();
            break;
        }

        recvPayloadBuffer_.clear();
        VERIFY(recvBuffer_.ReadToUInt8Arr(
            recvPayloadBuffer_, static_cast<size_t>(packetSize)));

        uint8* payloadPtr = recvPayloadBuffer_.data();
        int payloadSize = packetSize;

        // Decrypt if necessary.
        if (payloadFlags & (uint8)EPayloadFlag::Encrypt)
        {
            int decryptedSize;
            if (!cryptoContext_.Decrypt(
                    payloadPtr, payloadSize, decryptBuffer_, decryptedSize))
            {
                LogError("Failed to decrypt.");
                return;
            }

            payloadPtr = decryptBuffer_.data();
            payloadSize = decryptedSize;
        }

        // Uncompress if necessary.
        if (payloadFlags & (uint8)EPayloadFlag::Compress)
        {
            // TODO uncompressedSize 사이즈를 최적화를 위해 클라에서 uncompressedSize
            // 받아야 됨. this.SendJsonPacket 와 같이.
            int uncompressedSize = TEMP_BUF_SIZE;
            uncompressBuffer_.clear();
            uncompressBuffer_.reserve(uncompressedSize);

            int zerr = uncompress(uncompressBuffer_.data(), &(uLong)uncompressedSize,
                payloadPtr, (uLong)payloadSize);

            if (zerr != Z_OK)
            {
                LogError("Failed to uncompress.");
                return;
            }

            payloadPtr = uncompressBuffer_.data();
            payloadSize = uncompressedSize;
        }

        recvBuffer_.CommitRead();

        if (payloadFlags & (uint8)EPayloadFlag::Binary)
        {
            ByteBuffer buf(payloadSize);
            buf.Write(payloadPtr, payloadSize);
            ProcessBinaryPacket(buf);
        }
        else
        {
            ProcessJsonPacket(payloadPtr, payloadSize);
        }
    }
}

void PacketHandler::ProcessBinaryPacket(ByteBuffer& buffer)
{
    uint32 packetType;
    buffer.ReadUInt32LE(packetType);
    buffer.CommitRead();

    switch (packetType)
    {
    case EPacketType::TOWN_MOVE_CS:
        ProcessTownMoveBinaryPacket(buffer);
        break;
    }
}

void PacketHandler::ProcessTownMoveBinaryPacket(ByteBuffer& buffer)
{
    Json::Value bodyRoot;
    int x, y, degrees, speed;
    buffer.ReadInt32LE(x);
    buffer.ReadInt32LE(y);
    buffer.ReadInt32LE(degrees);
    buffer.ReadInt32LE(speed);
    bodyRoot["x"] = x;
    bodyRoot["y"] = y;
    bodyRoot["degrees"] = degrees;
    bodyRoot["speed"] = speed;

    Json::Value root;
    root["type"] = static_cast<int>(EPacketType::TOWN_MOVE_CS);
    root["seqNum"] = 0;
    root["body"] = json::Stringify(bodyRoot);

    if (!ProcessPacket(EPacketType::TOWN_MOVE_CS, 0, bodyRoot))
    {
        client_->PacketUnknown(json::Stringify(root));
    }
}

void PacketHandler::ProcessJsonPacket(uint8* payloadPtr, int payloadSize)
{
    std::string s(reinterpret_cast<const char*>(payloadPtr), payloadSize);

    Json::Value root;
    json::Parse(s, root);

    Log("Packet received. Packet: %s", s.c_str());

    int packetType = root.get("type", "").asInt();
    int seqNum = root.get("seqNum", "").asInt();

    Json::Value bodyRoot;
    json::Parse(root["body"].asString(), bodyRoot);

    if (!ProcessPacket(static_cast<EPacketType>(packetType), seqNum, bodyRoot))
    {
        client_->PacketUnknown(s);
    }
}

bool PacketHandler::ProcessPacket(
    EPacketType packetType, int seqNum, const Json::Value& bodyRoot)
{
    IClientPacketHandler* handler = GetClientPacketHandler(packetType);
    handler->Exec(this, seqNum, bodyRoot);
    return true;
}

IClientPacketHandler* PacketHandler::GetClientPacketHandler(EPacketType packetType)
{
    std::map<EPacketType, IClientPacketHandler*>::iterator it
        = packetHandlers_.find(packetType);

    if (it != packetHandlers_.end())
    {
        return it->second;
    }

    IClientPacketHandler* handler;

    switch (packetType)
    {
    case EPacketType::AUTH_HELLO:
        handler = new CphAuthHello();
        break;
    case EPacketType::AUTH_LOGIN:
        handler = new CphAuthLogin();
        break;
    case EPacketType::AUTH_ENTER_WORLD:
        handler = new CphAuthEnterWorld();
        break;
    case EPacketType::TOWN_MOVE_CS:
        handler = new CphTownMove();
        break;
    case EPacketType::DEV_SET_POINT:
        handler = new CphDevSetPoint();
        break;
    default:
        ASSERT(false);
    }

    packetHandlers_.insert(
        std::pair<EPacketType, IClientPacketHandler*>(packetType, handler));

    return handler;
}
