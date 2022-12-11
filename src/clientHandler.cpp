#include "clientHandler.h"
#include "packetHandler/packetHandler.h"
#include "serverInstance.h"
#include "world.h"
#include "json/json.h"
#include <chrono>

static const std::chrono::milliseconds PING_TIME_MS = std::chrono::milliseconds(1000);

int ClientHandler::clientCount_ = 0;

ClientHandler::ClientHandler(const std::string& ip)
    : ip_(ip)
    , player_(nullptr)
    , bHasSentDisconnect_(false)
    , ping_(1000)
    , pingId_(1)
    , connectionState_(EConnectionState::JustConnected)
    , uniqueId_(0)
    , bHasPrintTickThreadId_(false)
{
    packetHandler_ = std::make_unique<PacketHandler>(this);
    uniqueId_ = clientCount_++;
    pingStartTime_ = std::chrono::steady_clock::now();

    Log("New ClientHandler created at %p", static_cast<void*>(this));
}

ClientHandler::~ClientHandler()
{
    ASSERT(connectionState_ == EConnectionState::LoggedOut);

    Log("Deleting client \"%d\" at %p", id_, static_cast<void*>(this));

    Log("ClientHandler at %p deleted", static_cast<void*>(this));
}

void ClientHandler::Destroy()
{
    if (connectionState_ == EConnectionState::LoggedOut)
    {
        // Already called.
        LogError("%s: client %p, \"%d\" already destroyed.", __FUNCTION__,
            static_cast<void*>(this), id_);
        return;
    }

    Log("%s: destroying client %p, \"%d\" @ %s", __FUNCTION__, static_cast<void*>(this),
        id_, ip_.c_str());

    {
        CSLock lock(csOutgoingData_);
        tcpConn_->Shutdown();
        tcpConn_.reset();
    }
}

void ClientHandler::ServerTick(float delta)
{
    ProcessRecv();

    // TODO ping-pong
}

void ClientHandler::WorldTick(float delta)
{
    if (IsLoggedOut())
    {
        return;
    }

    ProcessRecv();
}

void ClientHandler::SendData(const std::basic_string_view<std::byte> data)
{
    if (bHasSentDisconnect_)
    {
        return;
    }

    // null check 와 send 사이에 리셋되는 것을 방지하기 위해 캡쳐.
    if (auto conn = tcpConn_; conn != nullptr)
    {
        CSLock lock(csOutgoingData_);
        conn->Send(data.data(), data.size());
    }
}

void ClientHandler::SendData(const void* data, size_t length)
{
    if (bHasSentDisconnect_)
    {
        return;
    }

    // null check 와 send 사이에 리셋되는 것을 방지하기 위해 캡쳐.
    if (auto conn = tcpConn_; conn != nullptr)
    {
        CSLock lock(csOutgoingData_);
        conn->Send(data, length);
    }
}

void ClientHandler::SendDisconnect(const std::string& reason)
{
    if (!bHasSentDisconnect_)
    {
        Log("Sending disconnect: \"%s\"", reason.c_str());
        // TODO SendJsonPacket

        bHasSentDisconnect_ = true;
        Destroy();
    }
}

void ClientHandler::PacketBufferFull()
{
    // Incoming queue 에 데이터가 너무 많은 경우 클라를 킥한다.
    LogError("Incomming packet buffer full. Id: %d, IP: %s.", id_, ip_.c_str());
    SendDisconnect("Server is busy.");
}

void ClientHandler::PacketUnknown(const std::string& packetData)
{
    LogError("Unknown packet type. Packet: %s, IP: %s", packetData.c_str(), ip_.c_str());

    std::string reason = "Unknown packet type";

    SendDisconnect(reason);
}

void ClientHandler::PacketError(int packetType, const std::string& errMsg)
{
    LogError("Packet processed with error. Packet type: %d, ErrMsg: %s", packetType,
        errMsg.c_str());

    // TODO 에러 종류에 따라 분기 처리.
    SendDisconnect(errMsg);
}

void ClientHandler::OnHello()
{

    CSLock lock(csConnctionState_);
    if (connectionState_ != EConnectionState::JustConnected)
    {
        LogError(
            "%s: Invalid connection state(%d)", __FUNCTION__, connectionState_.load());
        return;
    }

    connectionState_ = EConnectionState::KeysExchanged;
}

void ClientHandler::OnLogin(bool bIsNewUser)
{
    CSLock lock(csConnctionState_);
    if (connectionState_ != EConnectionState::KeysExchanged)
    {
        LogError(
            "%s: Invalid connection state(%d)", __FUNCTION__, connectionState_.load());
        return;
    }

    // TODO

    Json::Value bodyRoot;
    bodyRoot["bIsNewUser"] = bIsNewUser;

    // TODO packet hander 으로 이동.
    packetHandler_->SendJsonPacket(EPacketType::AUTH_LOGIN, 0, bodyRoot);

    connectionState_ = EConnectionState::LoggingIn;
}

void ClientHandler::OnEnterWorld(int seqNum, const UserDb::EnterWorldResult& ret)
{
    {
        CSLock lock(csConnctionState_);
        if (connectionState_ != EConnectionState::LoggingIn)
        {
            LogError("%s: Invalid connection state(%d)", __FUNCTION__,
                connectionState_.load());
            return;
        }

        // TODO
    }

    auto player = std::make_unique<Player>(shared_from_this());
    player_ = player.get();

    // TODO load world
    World* world = new World();

    // TODO
    Json::Value bodyRoot;
    bodyRoot["userId"] = ret.userId;
    bodyRoot["name"] = ret.name;
    bodyRoot["exp"] = ret.exp;
    bodyRoot["level"] = ret.level;

    // TODO packet hander 으로 이동.
    packetHandler_->SendJsonPacket(EPacketType::AUTH_ENTER_WORLD, seqNum, bodyRoot);

    // Add to world.
    player_->Initialize(std::move(player), *world);

    ServerInstance::Get().GetServer()->OnClientMovedToWorld(this);
    bHasPrintTickThreadId_ = false;
    world->Start();

    {
        CSLock lock(csConnctionState_);
        connectionState_ = EConnectionState::InWorld;
    }
}

void ClientHandler::SocketClosed()
{
    Log("Socket Closed. Id: %d, IP: %s, (%p), state: %d, m_Player: %p", id_, ip_.c_str(),
        static_cast<void*>(this), connectionState_.load(), static_cast<void*>(player_));

    Destroy();
}

void ClientHandler::ProcessRecv()
{
    // TEMP
    if (!bHasPrintTickThreadId_)
    {
        bHasPrintTickThreadId_ = true;
        std::cout << "ClientHandler tick thread id: " << std::this_thread::get_id()
                  << std::endl;
    }

    std::string incomingData;
    {
        CSLock lock(csIncomingData_);
        std::swap(incomingData, incomingData_);
    }

    if (incomingData.empty())
    {
        return;
    }

    packetHandler_->ReadPayloads(incomingData);
}

void ClientHandler::OnConnCreated(std::shared_ptr<ITCPConnection> conn)
{
    tcpConn_ = conn;
}

void ClientHandler::OnReceivedData(const char* data, size_t length)
{
    CSLock lock(csIncomingData_);
    incomingData_.append(data, length);
}

void ClientHandler::OnRemoteClosed()
{
    Log("Client socket closed. Id: %d, IP: @ %s", id_, ip_.c_str());
    SocketClosed();
}

void ClientHandler::OnError(int errorCode, const std::string& errorMsg)
{
    LogError("ClientHandler::OnError. Id: %d, IP: %s, err code: %d, err msg: %s. Client "
             "disconnected.",
        id_, ip_.c_str(), errorCode, errorMsg.c_str());
    SocketClosed();
}