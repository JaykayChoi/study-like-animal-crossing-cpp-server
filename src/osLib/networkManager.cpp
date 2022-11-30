#include "networkManager.h"
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/thread.h>

NetworkManager::NetworkManager()
    : bHasTerminated_(true)
{
}

NetworkManager::~NetworkManager() noexcept(false)
{
    // Terminate() 가 호출되었는지 확인.
    ASSERT(bHasTerminated_);
}

NetworkManager& NetworkManager::Get()
{
    static NetworkManager instance;
    return instance;
}

void NetworkManager::Initialize()
{

}

void NetworkManager::Terminate()
{

}

void NetworkManager::AddListenServer(const std::shared_ptr<IListenServer>& server)
{

}

void NetworkManager::RemoveListenServer(const IListenServer* server)
{

}

void NetworkManager::LogCallback(int severity, const char* msg)
{

}

void NetworkManager::RunEventLoop(NetworkManager* self)
{

}

void NetworkManager::EventLoopCallback(evutil_socket_t socket, short events, void* self)
{

}

