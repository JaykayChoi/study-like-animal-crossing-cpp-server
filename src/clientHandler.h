#pragma once

#include "global.h"
#include "osLib/network.h"

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

private:
    // TODO jaykay

    std::string ip_;

    std::unique_ptr<PacketHandler> packetHandler_;

    // TODO jaykay
};
