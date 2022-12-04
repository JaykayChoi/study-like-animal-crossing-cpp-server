#include "server.h"
#include "clientHandler.h"

/////////////////////////////////////////////////////////////////////////////////////////
// ServerListenCallback

class ServerListenCallback : public Network::ListenCallback
{
	Server& server_;
	int port_;

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

public:
	ServerListenCallback(Server& server, int port)
		: server_(server)
		, port_(port)
	{
	}
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

	// TODO config
	ports_.push_back(20100);
	serverId_ = "temp";
	maxPlayers_ = 30;

	bIsConnected_ = true;

	return true;
}

bool Server::Start()
{
	for (const auth& port : ports_)
	{
		Log("Socket start listening... (port: %d)", port);
		auto listenServer = Network::Listen(port, std::make_shared<ServerListenCallback>(*this, port));
	}
}

void Server::Shutdown()
{

}

void Server::OnClientMovedToWorld(const ClientHandler* client)
{

}

std::shared_ptr <ITCPConnection::Callback> Server::OnConnectionAccepted(const std::string& remoteIPAddress)
{

}

void Server::Tick(float delta)
{

}

void Server::TickClients(float delta)
{

}