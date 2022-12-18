#pragma once

#include "../global.h"
#include "criticalSection.h"
#include "network.h"
#include <event2/listener.h>
#include "wEvent.h"

// TEMP
#define TCP_CONN_EVENT_BASE_MULTI_THREAD

class TCPConnection;

class ListenServer;

class ListenServer : public IListenServer
{
    using Super = IListenServer;
    friend class TCPConnection;

public:
    // 서버를 닫고 모든 연결을 드랍시킨다.
    virtual ~ListenServer() override;

    /**
     * LIsten
     * IPv4, IPv6 모두 사용 가능.
     * 항상 callback 을 반환. 오류 발생시 callback 에 오류 정보가 있음.
     */
    static std::shared_ptr<ListenServer> Listen(
        int port, std::shared_ptr<Network::ListenCallback> listenCallback);

    // IListenServer overrides.
    virtual void Close() override;
    virtual bool IsListening() const override { return bIsListening_; }

protected:
    std::shared_ptr<Network::ListenCallback> listenCallback_;

    evconnlistener* connListener_;

    bool bIsListening_;

    // 활성화된 모든 연결들.
    std::vector<std::shared_ptr<TCPConnection>> connections_;

    // connections_ 보호.
    CriticalSection cs_;

    // 리스닝 실패에 대한 에러코드. 실패한 경우에만 유효함.
    int errorCode_;

    // 리스닝 실패에 대한 에러 메시지. 실패한 경우에만 유효함.
    std::string errorMsg_;

    // ListenServer::Listen() 에서 생성된 std::shared_ptr<ListenServer>.
    std::shared_ptr<ListenServer> selfPtr_;

    /**
     * 콜백을 사용하여 새 인스턴스를 만든다.
     * 내부를 초기화하지만 아직 리스닝을 시작하지 않음.
     */
    ListenServer(std::shared_ptr<Network::ListenCallback> listenCallback);

    bool Listen(int port);

    static void OnConnected(evconnlistener* listener, evutil_socket_t socket,
        sockaddr* addr, int len, void* inSelf);

    /**
     * connections_에서 conn 제거.
     * 연결이 종료되면 TCPConnection에 의해 호출됩니다.
     */
    void RemoveConn(const TCPConnection* conn);

#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
    std::vector<event_base*> tcpConnEventBases;
    std::vector<std::thread> tcpConnEventLoopThreads;

    WEvent startupTCPConnEvent_;

    void CreateTCPConnEventBases(std::shared_ptr<ListenServer> self);

    event_base* GetTCPConnEventBase(int64 connCount);

    static void RunTCPConnEventLoop(std::shared_ptr<ListenServer> self, event_base* base);
    static void TCPConnEventLoopCallback(
        evutil_socket_t socket, short events, void* self);
#endif
};