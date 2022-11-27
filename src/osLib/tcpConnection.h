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

	// TODO
};