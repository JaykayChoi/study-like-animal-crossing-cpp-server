#pragma once

#include "global.h"
#include "osLib/criticalSection.h"
#include "osLib/network.h"
#include "osLib/thread.h"

class ClientHandler;

class Server
{
public:
    virtual ~Server() { }

    bool Init();

    bool Start();

    void Shutdown();

    /**
     * 클라가 world 로 이동된 경우 호출된다.
     * 이후 ClientHandler 의 tick 은 world 에서 호출한다.
     */
    void OnClientMovedToWorld(const ClientHandler* client);

private:
    friend class ServerInstance;
    friend class ServerListenCallback;

    class TickThread : public Thread
    {
        using Super = Thread;

    public:
        TickThread(Server& server);

    protected:
        Server& server_;

        // Thread overrides.
        virtual void Execute() override;
    };

    std::vector<std::shared_ptr<IListenServer>> listenServers_;

    // Protects clients_, clientsToRemove_
    CriticalSection csClients_;

    // 연결된 클라들.
    std::list<std::shared_ptr<ClientHandler>> clients_;

    // 다음 Tick에 m_Clients 에서 제거될 클라들.
    std::list<ClientHandler*> clientsToRemove_;

    std::atomic_size_t playerCount_;
    size_t maxPlayers_;

    bool bIsConnected_;

    std::vector<int> ports_;

    std::string serverId_;

    TickThread tickThread_;

    Server();

    // ClientHandle 인스턴스를 만들고 클라이언트 목록에 추가한다.
    std::shared_ptr<ITCPConnection::Callback> OnConnectionAccepted(
        const std::string& remoteIPAddress);

    void Tick(float delta);

    void TickClients(float delta);
};