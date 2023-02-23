#pragma once

#include "global.h"
#include "osLib/criticalSection.h"
#include "osLib/thread.h"

class Actor;

class World
{
public:
    World();

    ~World();

    void Start();

    void Stop();

    void AddActor(std::unique_ptr<Actor> actor);

    void AddClientHandler(ClientHandler* client); // 임시로 월드에 위치.

    void BroadcastActorPosition(const Actor& actor, const ClientHandler* exclude);

private:
    class TickThread : public Thread
    {
        using Super = Thread;

    public:
        TickThread(World& world);

    protected:
        World& world_;

        // Thread overrides.
        virtual void Execute() override;
    };

    TickThread tickThread_;

    CriticalSection csActors_;
    std::vector<std::unique_ptr<Actor>> actors_;

    // TODO 월드 chunk 단위로 분리하여 해당 class 로 이동.
    CriticalSection csClients_;
    std::vector<ClientHandler*> clientHandlers_;

    void Tick(float delta);

    std::vector<uint8> BuildTownMovePacketBuffer(const Actor& actor);
};
