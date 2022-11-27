#include "listenServer.h"
#include "../global.h"
#include "tcpConnection.h"

#ifdef TCP_CONN_EVENT_BASE_MULTI_THREAD
// TODO config (TCPConnImpl::ReadCallback (per conn))
const int MAX_TCP_CONN_EVENT_BASE_IO_THREAD_COUNT = 5;
#endif

namespace ListenServerImplHelper
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
        // TODO
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
    // TODO
}

bool ListenServer::Listen(int port)
{
    // TODO
    return false;
}

void ListenServer::OnConnected(
    evconnlistener* listener, evutil_socket_t socket, sockaddr* addr, int len, void* self)
{
    // TODO
}

void ListenServer::RemoveConn(const TCPConnection* conn)
{
    // TODO
}

void ListenServer::CreateTCPConnEventBases(std::shared_ptr<ListenServer> self)
{
    // TODO
}

event_base* ListenServer::GetTCPConnEventBase(int64 connCount)
{
    // TODO
    return nullptr;
}

void ListenServer::RunTCPConnEventLoop(
    std::shared_ptr<ListenServer> self, event_base* base)
{
    // TODO
}

void ListenServer::TCPConnEventLoopCallback(
    evutil_socket_t socket, short events, void* self)
{
    // TODO
}
