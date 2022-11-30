#include "tcpConnection.h"
#include "event2/buffer.h"
#include "listenServer.h"

TCPConnection::TCPConnection(evutil_socket_t socket,
    std::shared_ptr<ITCPConnection::Callback> tcpConnCallback, event_base* eventBase,
    std::shared_ptr<ListenServer> server, const sockaddr* address, socklen_t addrLen)
    : Super(std::move(tcpConnCallback))
    , bufferEvent_(bufferevent_socket_new(eventBase, socket,
          BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE | BEV_OPT_DEFER_CALLBACKS
              | BEV_OPT_UNLOCK_CALLBACKS))
    , server_(std::move(server))
    , localPort_(0)
    , remotePort_(0)
    , bShouldShutdown_(false)
{
    UpdateLocalAddress();
    UpdateAddress(address, addrLen, remoteIP_, remotePort_);
}

TCPConnection::~TCPConnection() { bufferevent_free(bufferEvent_); }

void TCPConnection::Enable(std::shared_ptr<TCPConnection> self)
{
    // 콜백이 오는 동안 유지하기 위해 self instance 를 가지고 있는다.
    selfPtr_ = std::move(self);

    bufferevent_setcb(bufferEvent_, ReadCallback, WriteCallback, EventCallback, this);
    bufferevent_enable(bufferEvent_, EV_READ | EV_WRITE);
}

bool TCPConnection::Send(const void* data, size_t length)
{
    if (bShouldShutdown_)
    {
        LogError("%s: Cannot send data, connection is already shutdown.", __FUNCTION__);
        return false;
    }

    return SendRaw(data, length);
}

void TCPConnection::Shutdown()
{
    // 나가는 데이터가 없으면 소켓을 직접 종료한다.
    if (evbuffer_get_length(bufferevent_get_output(bufferEvent_)) == 0)
    {
        DoActualShutdown();
        return;
    }

    // libevent 버퍼에 나가는 데이터가 존재. OS 의 TCP 스택에 기록될 때 종료를 예약.
    bShouldShutdown_ = true;
}

void TCPConnection::Close()
{
    // keep alive 상태로 소켓의 모든 이벤트를 disable 시킨다.
    bufferevent_disable(bufferEvent_, EV_READ | EV_WRITE);

    server_->RemoveConn(this);
    selfPtr_.reset();
}

void TCPConnection::ReadCallback(bufferevent* bufferEvent, void* inSelf)
{
    std::cout << "TCPConnImpl::ReadCallback (per conn) thread id: "
              << std::this_thread::get_id() << std::endl;

    ASSERT(inSelf != nullptr);
    TCPConnection* self = static_cast<TCPConnection*>(inSelf);
    ASSERT(self->bufferEvent_ == bufferEvent);
    ASSERT(self->callback_ != nullptr);

    // 1024 byte 청크로 들어오는 데이터를 리드.
    char data[1024];
    size_t length;
    while ((length = bufferevent_read(bufferEvent, data, sizeof(data))) > 0)
    {
        self->OnReceivedCleartextData(data, length);
    }
}

void TCPConnection::WriteCallback(bufferevent* bufferEvent, void* inSelf)
{
    ASSERT(inSelf != nullptr);
    auto self = static_cast<TCPConnection*>(inSelf);
    ASSERT(self->callback_ != nullptr);

    // 더 이상 쓸 데이터가 없고 연결이 종료되도록 예약된 경우 종료 수행.
    auto outlen = evbuffer_get_length(bufferevent_get_output(self->bufferEvent_));
    if ((outlen == 0) && (self->bShouldShutdown_))
    {
        self->DoActualShutdown();
    }
}

void TCPConnection::EventCallback(bufferevent* bufferEvent, short what, void* inSelf)
{
    ASSERT(inSelf != nullptr);
    std::shared_ptr<TCPConnection> self = static_cast<TCPConnection*>(inSelf)->selfPtr_;
    // TODO
}