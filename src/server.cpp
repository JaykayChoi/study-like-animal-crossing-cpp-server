#include "server.h"
#include "clientHandler.h"

/////////////////////////////////////////////////////////////////////////////////////////
// ServerListenCallback

class ServerListenCallback : public Network::ListenCallback
{
public:
	ServerListenCallback(Server& server, int port)
		: server_(server)
		, port_(port)
	{
	}

    virtual std::shared_ptr<ITCPConnection::Callback> OnIncomingConnection(
        const std::string& remoteIPAddress, int remotePort) override
    {
        return server_.OnConnectionAccepted(remoteIPAddress);
    }

    virtual void OnAccepted(ITCPConnection& conn) override { }

    virtual void OnError(int errorCode, const std::string& errorMsg) override
    {
        LogWarn("Cannot listen on port %d: %d (%s).", port_, errorCode, errorMsg.c_str());
    }

private:
	Server& server_;
	int port_;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Server::TickThread

Server::TickThread::TickThread(Server& server)
    : Super("ServerTickThread")
    , server_(server)
{
}

void Server::TickThread::Execute()
{
    auto lastTime = std::chrono::steady_clock::now();
    static const auto msPerTick = std::chrono::milliseconds(50);

    while (!bShouldTerminate_)
    {
        auto nowTime = std::chrono::steady_clock::now();
        auto delta
            = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastTime)
                  .count();
        server_.Tick(static_cast<float>(delta));
        auto tickTime = std::chrono::steady_clock::now() - nowTime;

        if (tickTime < msPerTick)
        {
            // 최소 msPerTick이 될 때 까지 sleep
            std::this_thread::sleep_for(msPerTick - tickTime);
        }

        lastTime = nowTime;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Server

Server::Server()
    : playerCount_(0)
    , maxPlayers_(0)
    , bIsConnected_(false)
    , tickThread_(*this)
{
}

bool Server::Init()
{
    if (bIsConnected_)
    {
        LogError("Server is already running.");
        return false;
    }

    // TODO config 로 빼야 됨. 
    ports_.push_back(20100);
    serverId_ = "temp";
    maxPlayers_ = 30;

    bIsConnected_ = true;

    return true;
}

bool Server::Start()
{
    for (const auto& port : ports_)
    {
        Log("Socket start listening... (port: %d)", port);
        auto listenServer
            = Network::Listen(port, std::make_shared<ServerListenCallback>(*this, port));
        if (listenServer->IsListening())
        {
            listenServers_.push_back(listenServer);
        }
    }

    if (listenServers_.empty())
    {
        LogError("Cannot open any ports.");
        return false;
    }
    return tickThread_.Start();
}

void Server::Shutdown()
{
    for (const auto& elem : listenServers_)
    {
        elem->Close();
    }
    listenServers_.clear();

    tickThread_.Stop();

    CSLock lock(csClients_);
    for (auto itr = clients_.begin(); itr != clients_.end(); itr++)
    {
        (*itr)->Destroy();
    }
    clients_.clear();
}

void Server::OnClientMovedToWorld(const ClientHandler* client)
{
    CSLock lock(csClients_);
    clientsToRemove_.push_back(const_cast<ClientHandler*>(client));
}

std::shared_ptr<ITCPConnection::Callback> Server::OnConnectionAccepted(
    const std::string& remoteIPAddress)
{
    Log("Client %s connected.", remoteIPAddress.c_str());
    std::shared_ptr<ClientHandler> newHandler
        = std::make_shared<ClientHandler>(remoteIPAddress);
    CSLock lock(csClients_);
    clients_.push_back(newHandler);
    return std::move(newHandler);
}

void Server::Tick(float delta) { TickClients(delta); }

void Server::TickClients(float delta)
{
    std::list<std::shared_ptr<ClientHandler>> loggedOutClients;
    {
        CSLock lock(csClients_);

        // clientsToRemove_ 에 속한 ClientHandler 들 제거.
        for (auto itr = clientsToRemove_.begin(); itr != clientsToRemove_.end(); itr++)
        {
            for (auto itrC = clients_.begin(); itrC != clients_.end(); itrC++)
            {
                if (itrC->get() == *itr)
                {
                    clients_.erase(itrC);
                    break;
                }
            }
        }
        clientsToRemove_.clear();

        // tick
        for (auto itr = clients_.begin(); itr != clients_.end();)
        {
            auto& client = *itr;

            // TODO multi-thread
            client->ServerTick(delta);

            if (client->IsLoggedOut())
            {
                // 데드락을 피하기 위해 락이 풀린 이후에 삭제.
                loggedOutClients.push_back(std::move(client));
                itr = clients_.erase(itr);
                continue;
            }

            itr++;
        }
    }

    loggedOutClients.clear();
}