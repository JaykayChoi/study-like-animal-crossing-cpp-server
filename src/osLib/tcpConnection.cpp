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
        self->OnReceivedData(data, length);
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
    
	if (self == nullptr)
	{
		// 이미 해제된 상태.
		return;
	}

	if (what && BEV_EVENT_ERROR)
	{
		// 연결을 기다리고 있는지 여부에 따라 호출할 콜백 선택.
		int err = EVUTIL_SOCKET_ERROR();
		
		self->callback_->OnError(err, evutil_socket_error_to_string(err));
		self->server_->RemoveConn(self.get());
		self->selfPtr_.reset();
		return;
	}

	// 연결이 종료된 경우 콜백을 호출하고 연결 제거.
	if (what & BEV_EVENT_EOF)
	{
		self->callback_->OnRemoteClosed();
		if (self->server_ != nullptr)
		{
			self->server_->RemoveConn(self.get());
		}

		self->selfPtr_.reset();
		return;
	}

	// Unknown event.
	LogWarn("%s: Unhandled LibEvent event %d (0x%x)", __FUNCTION__, what, what);
	ASSERT(!"TCPConnection::EventCallback: Unhandled LibEvent event");
}

void TCPConnection::UpdateAddress(const sockaddr* address, socklen_t addrLen, std::string& inIP, int& port)
{
	// Convert to IP.
	char ip[128];

	switch (address->sa_family)
	{
	case AF_INET: // IPv4:
	{
		const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*>(address);
		evutil_inet_ntop(AF_INET, &(sin->sin_addr), ip, sizeof(ip));
		port = ntohs(sin->sin_port);
		break;
	}
	case AF_INET6: // IPv6
	{
		const sockaddr_in6* sin = reinterpret_cast<const sockaddr_in6*>(address);
		evutil_inet_ntop(AF_INET6, &(sin->sin6_addr), ip, sizeof(ip));
		port = ntohs(sin->sin6_port);
		break;
	}

	default:
	{
		LogWarn(
			"%s: Unknown socket address family: %d", __FUNCTION__, address->sa_family);
		ASSERT(!"Unknown socket address family");
		break;
	}
	}
	inIP.assign(ip);
}

void TCPConnection::UpdateLocalAddress()
{
	sockaddr_storage sa;
	socklen_t salen = static_cast<socklen_t>(sizeof(sa));
	getsockname(
		bufferevent_getfd(bufferEvent_), reinterpret_cast<sockaddr*>(&sa), &salen);
	UpdateAddress(reinterpret_cast<const sockaddr*>(&sa), salen, localIP_, localPort_);
}

void TCPConnection::UpdateRemoteAddress()
{
	sockaddr_storage sa;
	socklen_t salen = static_cast<socklen_t>(sizeof(sa));
	getpeername(
		bufferevent_getfd(bufferEvent_), reinterpret_cast<sockaddr*>(&sa), &salen);
	UpdateAddress(reinterpret_cast<const sockaddr*>(&sa), salen, remoteIP_, remotePort_);
}

void TCPConnection::DoActualShutdown()
{
	shutdown(bufferevent_getfd(bufferEvent_), SD_SEND);

	bufferevent_disable(bufferEvent_, EV_WRITE);
}

bool TCPConnection::SendRaw(const void* data, size_t length)
{
	return (bufferevent_write(bufferEvent_, data, length) == 0);
}

void TCPConnection::OnReceivedData(const char* data, size_t length)
{
	ASSERT(callback_ != nullptr);
	callback_->OnReceivedData(data, length);
}
