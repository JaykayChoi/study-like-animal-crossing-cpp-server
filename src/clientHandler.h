#pragma once

#include "actor/player.h"
#include "global.h"
#include "osLib/criticalSection.h"
#include "osLib/network.h"
#include "sql/userDb.h"

enum class EConnectionState : uint8
{
    JustConnected,
    KeysExchanged,
    LoggingIn,
    LoggedIn,
    InWorld,
    LoggedOut,
};

class PacketHandler;

class ClientHandler : public ITCPConnection::Callback,
                      // 같은 raw pointer 를 포인팅하는 shared_ptr 을 만들기 위해.
                      public std::enable_shared_from_this<ClientHandler>
{
public:
    ClientHandler(const std::string& ip);

    virtual ~ClientHandler() override;

    const std::string& GetIP() const { return ip_; }

    void SetIP(const std::string& ip) { ip_ = ip; }

    // TODO

    void Destroy();

    // 월드에 입장하기 전 까지 tick
    void ServerTick(float delta);

    // 월드 입장 후 tick
    void WorldTick(float delta);

    inline bool IsLoggedIn() const
    {
        return connectionState_ >= EConnectionState::LoggedIn;
    }

    bool IsLoggedOut() const { return (connectionState_ == EConnectionState::LoggedOut); }

    inline short GetPing() const
    {
        return static_cast<short>(
            std::chrono::duration_cast<std::chrono::milliseconds>(ping_).count());
    }

    void SendData(const ContiguousByteViewContainer data);
    void SendData(const void* data, size_t length);

    void SendDisconnect(const std::string& reason);

    void PacketBufferFull();
    void PacketUnknown(const std::string& packetData);
    // TODO error code
    void PacketError(int packetType, const std::string& errMsg);

    void OnHello();

    void OnLogin(bool bIsNewUser);

    void OnEnterWorld(int seqNum, const UserDb::EnterWorldResult& ret);

private:
    friend class Server;

    std::string ip_;

    std::unique_ptr<PacketHandler> packetHandler_;

    // Protect incommingData_
    CriticalSection csIncomingData_;

    // Tick() 에서 처리될 때까지 수신된 수신 데이터를 위한 queue.
    std::string incomingData_;

    // Protects outgoing data.
    CriticalSection csOutgoingData_;

    Player* player_;

    // Disconnect packet 이 양방향으로 전송된 경우 true.
    bool bHasSentDisconnect_;

    // 마지막으로 완료된 핑 이후 시간.
    std::chrono::steady_clock::duration ping_;

    // Last ping request id
    int pingId_;

    // 클라에서 마지막으로 전송된 ping 요청 시간.
    std::chrono::steady_clock::time_point pingStartTime_;

    std::atomic<EConnectionState> connectionState_;
    CriticalSection csConnctionState_;

    static int clientCount_;

    // 인증에 사용되는 id. 순차 발급됨.
    int uniqueId_;

    int id_;

    std::shared_ptr<ITCPConnection> tcpConn_;

    // TEMP
    bool bPrintThreadId_;

    void SocketClosed();

    // Network input buffer 의 데이터를 처리한다.
    // Called by ServerTick()
    void ProcessRecv();

    // ITCPConnection::Callback overrides.
    virtual void OnConnCreated(std::shared_ptr<ITCPConnection> conn) override;
    virtual void OnReceivedData(const char* data, size_t length) override;
    virtual void OnRemoteClosed() override;
    virtual void OnError(int errorCode, const std::string& errorMsg) override;
};
