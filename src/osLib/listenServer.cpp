#include "listenServer.h"
#include "../global.h"
#include "networkManager.h"
#include "tcpConnection.h"

#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
// TODO config (TCPConnection::ReadCallback (per connection))
const int MAX_TCP_CONN_EVENT_BASE_IO_THREAD_COUNT = 5;
#endif

namespace ListenServerHelper
{
static bool IsValidSocket(evutil_socket_t socket) { return (socket != INVALID_SOCKET); }
}

/////////////////////////////////////////////////////////////////////////////////////////
// ListenServer

ListenServer::ListenServer(std::shared_ptr<Network::ListenCallback> listenCallback)
    : listenCallback_(std::move(listenCallback))
    , connListener_(nullptr)
    , bIsListening_(false)
    , errorCode_(0)
{
}

ListenServer::~ListenServer()
{
    if (connListener_ != nullptr)
    {
        evconnlistener_free(connListener_);
    }
}

std::shared_ptr<ListenServer> ListenServer::Listen(
    int port, std::shared_ptr<Network::ListenCallback> listenCallback)
{
    std::shared_ptr<ListenServer> res { new ListenServer(std::move(listenCallback)) };
    res->selfPtr_ = res;

#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
    res->CreateTCPConnEventBases(res);
#endif

    if (res->Listen(port))
    {
        NetworkManager::Get().AddListenServer(res);
    }
    else
    {
        res->listenCallback_->OnError(res->errorCode_, res->errorMsg_);
        res->selfPtr_.reset();
    }

    return res;
}

void ListenServer::Close()
{
    if (connListener_ != nullptr)
    {
        evconnlistener_disable(connListener_);
    }

    bIsListening_ = false;

    // Shutdown all connections.
    std::vector<std::shared_ptr<TCPConnection>> conns;
    {
        CSLock lock(cs_);
        std::swap(conns, connections_);
    }
    for (const auto& conn : conns)
    {
        conn->Shutdown();
    }

#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
    for (int i = 0; i < MAX_TCP_CONN_EVENT_BASE_IO_THREAD_COUNT; i++)
    {
        event_base_loopbreak(tcpConnEventBases[i]);
        tcpConnEventLoopThreads[i].join();
        event_base_free(tcpConnEventBases[i]);
    }
#endif

    selfPtr_.reset();

    NetworkManager::Get().RemoveListenServer(this);
}

bool ListenServer::Listen(int port)
{
    std::cout << "ListenServerImpl::Listen (main) thread id: "
              << std::this_thread::get_id() << std::endl;

    // NetworkManager 가 초기화되어 있지 않을 경우 초기화.
    NetworkManager::Get();

    bool bNeedsIpv4Socket = true;
    int err = 0;

    // IPv6 socket 생성.
    evutil_socket_t sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

    // 가능한 경우 IPv4 fallback 을 사용하여 IPv6에서 수신한다.
    if (ListenServerHelper::IsValidSocket(sock))
    {
        // IPv6 socket 생성되고 dualstack mode 로 전환.
        uint32 zero = 0;
        int res = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
            reinterpret_cast<const char*>(&zero), sizeof(zero));
        err = EVUTIL_SOCKET_ERROR();
        bNeedsIpv4Socket = ((res == SOCKET_ERROR) && (err == WSAENOPROTOOPT));

        /**
         * http://wiki.pchero21.com/wiki/Libevent_R5:_Helper_functions_and_types_for_Libevent
         * Socket 종료될 경우 즉시 재사용이 가능하도록.
         */
        if (evutil_make_listen_socket_reuseable(sock) != 0)
        {
            errorCode_ = EVUTIL_SOCKET_ERROR();
            errorMsg_ = fmt::sprintf(
                "evutil_make_listen_socket_reuseable is failed. Port: %d, err code: %d (%s).\
					If Restart server, not work.",
                port, errorCode_, evutil_socket_error_to_string(errorCode_));
            Log("%s", errorMsg_.c_str());
        }

        // Bind.
        sockaddr_in6 name;
        memset(&name, 0, sizeof(name));
        name.sin6_family = AF_INET6;
        name.sin6_port = ntohs(port);
        if (bind(sock, reinterpret_cast<const sockaddr*>(&name), sizeof(name)) != 0)
        {
            errorCode_ = EVUTIL_SOCKET_ERROR();
            errorMsg_ = fmt::sprintf("Cannot bind IPv6 socket to port %d: %d (%s)", port,
                errorCode_, evutil_socket_error_to_string(errorCode_));
            Log("%s", errorMsg_.c_str());
            evutil_closesocket(sock);
            return false;
        }
    }

    if (bNeedsIpv4Socket)
    {
        // IPv6 socket 생성에 실패한 경우 대신 IPv4 로 만든다.
        err = EVUTIL_SOCKET_ERROR();
        LogError("Failed to create IPv6 socket: %d (%s)", err,
            evutil_socket_error_to_string(err));
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (!ListenServerHelper::IsValidSocket(sock))
        {
            errorCode_ = EVUTIL_SOCKET_ERROR();
            errorMsg_ = fmt::sprintf("Cannot create socket for port %d: %s", port,
                evutil_socket_error_to_string(errorCode_));
            Log("%s", errorMsg_.c_str());
            return false;
        }

        if (evutil_make_listen_socket_reuseable(sock) != 0)
        {
            errorCode_ = EVUTIL_SOCKET_ERROR();
            errorMsg_ = fmt::sprintf(
                "evutil_make_listen_socket_reuseable is failed. Port: %d, err code: %d (%s).\
					If Restart server, not work.",
                port, errorCode_, evutil_socket_error_to_string(errorCode_));
            Log("%s", errorMsg_.c_str());
        }

        // Bind.
        sockaddr_in name;
        memset(&name, 0, sizeof(name));
        name.sin_family = AF_INET;
        name.sin_port = ntohs(port);
        if (bind(sock, reinterpret_cast<const sockaddr*>(&name), sizeof(name)) != 0)
        {
            errorCode_ = EVUTIL_SOCKET_ERROR();
            errorMsg_ = fmt::sprintf("Cannot bind IPv4 socket to port %d: %s", port,
                evutil_socket_error_to_string(errorCode_));
            Log("%s", errorMsg_.c_str());
            evutil_closesocket(sock);
            return false;
        }
    }

    // Nonblocking 소켓으로 변경.
    // http://wiki.pchero21.com/wiki/Libevent_R5:_Helper_functions_and_types_for_Libevent
    if (evutil_make_socket_nonblocking(sock) != 0)
    {
        errorCode_ = EVUTIL_SOCKET_ERROR();
        errorMsg_ = fmt::sprintf("Cannot make socket on port %d non-blocking: %d (%s)",
            port, errorCode_, evutil_socket_error_to_string(errorCode_));
        Log("%s", errorMsg_.c_str());
        evutil_closesocket(sock);
        return false;
    }

    // Listen.
    if (listen(sock, SOMAXCONN) != 0)
    {
        errorCode_ = EVUTIL_SOCKET_ERROR();
        errorMsg_ = fmt::sprintf("Cannot listen on port %d: %d (%s)", port, errorCode_,
            evutil_socket_error_to_string(errorCode_));
        Log("%s", errorMsg_.c_str());
        evutil_closesocket(sock);
        return false;
    }

    connListener_ = evconnlistener_new(NetworkManager::Get().GetEventBase(), OnConnected,
        this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 0, sock);
    bIsListening_ = true;

    return true;
}

void ListenServer::OnConnected(evconnlistener* listener, evutil_socket_t socket,
    sockaddr* addr, int len, void* inSelf)
{
    std::cout << "ListenServerImpl::OnConnected (single) thread id: "
              << std::this_thread::get_id() << std::endl;

    ListenServer* self = static_cast<ListenServer*>(inSelf);
    ASSERT(self != nullptr);
    ASSERT(self->selfPtr_ != nullptr);

    // IP 와 포트를 가져온다.
    char ipAddress[128];
    int port = 0;
    switch (addr->sa_family)
    {
    case AF_INET:
    {
        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(addr);
        evutil_inet_ntop(AF_INET, &(sin->sin_addr), ipAddress, ARRAY_LENGTH(ipAddress));
        port = ntohs(sin->sin_port);
        break;
    }
    case AF_INET6:
    {
        sockaddr_in6* sin6 = reinterpret_cast<sockaddr_in6*>(addr);
        evutil_inet_ntop(
            AF_INET6, &(sin6->sin6_addr), ipAddress, ARRAY_LENGTH(ipAddress));
        port = ntohs(sin6->sin6_port);
        break;
    }
    }

    // OnIncomingConnection 콜백을 호출하여 사용할 ClientHandler 을 가져온다.
    std::shared_ptr<ITCPConnection::Callback> clientHandler
        = self->listenCallback_->OnIncomingConnection(ipAddress, port);
    if (clientHandler == nullptr)
    {
        // 연결을 끊는다.
        evutil_closesocket(socket);
        return;
    }

    // Create TCPConnection.
#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
    static int64 connCount = 0;
    event_base* eventBase = self->GetTCPConnEventBase(connCount++);
#else
    event_base* eventBase = NetworkManager::Get().GetEventBase();
#endif

    std::shared_ptr<TCPConnection> conn = std::make_shared<TCPConnection>(socket,
        clientHandler, eventBase, self->selfPtr_, addr, static_cast<socklen_t>(len));
    {
        CSLock lock(self->cs_);
        self->connections_.push_back(conn);
    }

    clientHandler->OnConnCreated(conn);
    conn->Enable(conn);

    self->listenCallback_->OnAccepted(*conn);
}

void ListenServer::RemoveConn(const TCPConnection* conn)
{
    CSLock lock(cs_);
    for (auto itr = connections_.begin(); itr != connections_.end(); itr++)
    {
        if (itr->get() == conn)
        {
            connections_.erase(itr);
            return;
        }
    }
}

#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
void ListenServer::CreateTCPConnEventBases(std::shared_ptr<ListenServer> self)
{
    for (int i = 0; i < MAX_TCP_CONN_EVENT_BASE_IO_THREAD_COUNT; i++)
    {
        event_base* base = event_base_new();
        tcpConnEventBases.push_back(base);
        std::thread eventLoopThread = std::thread(RunTCPConnEventLoop, self, base);
        tcpConnEventLoopThreads.push_back(std::move(eventLoopThread));
        startupTCPConnEvent_.Wait();
    }
}

event_base* ListenServer::GetTCPConnEventBase(int64 connCount)
{
    return tcpConnEventBases[connCount % MAX_TCP_CONN_EVENT_BASE_IO_THREAD_COUNT];
}

void ListenServer::RunTCPConnEventLoop(
    std::shared_ptr<ListenServer> self, event_base* base)
{
    auto timer = evtimer_new(base, TCPConnEventLoopCallback, self.get());
    timeval timeout {}; // Zero timeout.
    evtimer_add(timer, &timeout);
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
    event_free(timer);
}

void ListenServer::TCPConnEventLoopCallback(
    evutil_socket_t socket, short events, void* self)
{
    auto selfListenSever = static_cast<ListenServer*>(self);
    ASSERT(selfListenSever != nullptr);
    selfListenSever->startupTCPConnEvent_.Set();
}
#endif
