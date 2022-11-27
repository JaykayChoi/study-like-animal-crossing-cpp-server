#include "network.h"
#include "listenServer.h"

std::shared_ptr<IListenServer> Network::Listen(
    int port, std::shared_ptr<ListenCallback> listenCallback)
{
    return ListenServer::Listen(port, std::move(listenCallback));
}
