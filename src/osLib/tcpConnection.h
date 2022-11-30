#pragma once

#include "network.h"
#include <event2/bufferevent.h>
#include <event2/event.h>

class ListenServer;

class TCPConnection : public ITCPConnection
{
    using Super = ITCPConnection;

public:
    /**
     * Network::Listen() 을 통해 만들어지는 새로운 연결에 사용된다.
     * @param address addrLen 연결된 remote peer 정보
     * 생성된 연결은 비활성화되어 있기 때문에 통신을 시작하려면 Enable()을 호출해야한다.
     * ListenServer::Callback 에서 만들어진다.
     */
    TCPConnection(evutil_socket_t socket,
        std::shared_ptr<ITCPConnection::Callback> tcpConnCallback, event_base* eventBase,
        std::shared_ptr<ListenServer> server, const sockaddr* address, socklen_t addrLen);

    virtual ~TCPConnection() override;

    /**
     * 통신을 활성화한다.
     * 통신이 비활성화 된 상태로 생성되므로 생성 콜백을 먼저 호출 할 수 있다.
     * 그리고 이 메서드를 사용하여 정기적인 통신을 보고받을 수 있다.
     * @parma self 콜백이 오는 동안 소켓이 스스로를 유지할 수 있도록 사용된다.
     */
    void Enable(std::shared_ptr<TCPConnection> self);

    // ITCPConnection overrides.
    virtual bool Send(const void* data, size_t length) override;
    virtual std::string GetLocalIP() const override { return localIP_; }
    virtual int GetLocalPort() const override { return localPort_; }
    virtual std::string GetRemoteIP() const override { return remoteIP_; }
    virtual int GetRemotePort() const override { return remotePort_; }
    virtual void Shutdown() override;
    virtual void Close() override;

protected:
    bufferevent* bufferEvent_;

    // 들어오는 연결에만 유효. 나가는 연결에는 nullptr
    std::shared_ptr<ListenServer> server_;

    // Local endpoint IP.
    std::string localIP_;

    // Local endpoint port.
    int localPort_;

    // Remote endpoint IP
    std::string remoteIP_;

    // Remote endpoint port
    int remotePort_;

    /**
     * 콜백이 오는 동안 객체를 유지하는데 사용됨.
     * Enable() 에서 초기화되고 Clonse(), EventCallback(RemoteClosed) 에서 삭제 됨.
     */
    std::shared_ptr<TCPConnection> selfPtr_;

    /**
     * Shutdown() 이 호출되어 queue 에 있을 경우 true.
     * 더 이상 Send()를 통해 데이털르 보낼 수 없음. (버퍼에 있는 데이터 포함)
     * 데이터가 OS TCP stack 으로 전송되면 socket 이 종료 됨.
     */
    bool bShouldShutdown_;

    // Remote peer 에서 데이터가 들어올 때 libevent 가 호출하는 콜백.
    static void ReadCallback(bufferevent* bufferevent, void* inSelf);

    // Remote peer 가 더 많은 데이터를 수신할 수 있을 때 libevent 가 호출하는 콜백.
    static void WriteCallback(bufferevent* bufferevent, void* inSelf);

    // 소켓에 데이터과 관련이 없는 이벤트가 있을 때 libevent 가 호출하는 콜백.
    static void EventCallback(bufferevent* bufferEvent, short what, void* inSelf);

    // Address 로 ip, port 설정.
    static void UpdateAddress(
        const sockaddr* address, socklen_t addrLen, std::string& ip, int& port);

    void UpdateLocalAddress();

    void UpdateRemoteAddress();

    /**
     * TCPConnection 에서 shutdown을 호출하고 libevent 쓰기를 비활성화.
     * libevent 버퍼의 모든 데이터가 OS TCP 스택으로 전송되고 이전에 shutdown()이 호출 된
       후에 호출된다.
     */
    void DoActualShutdown();

    bool SendRaw(const void* data, size_t length);

    // 소켓에서 데이터가 들어올 때 호출 됨.
    void OnReceivedCleartextData(const char* data, size_t length);
};