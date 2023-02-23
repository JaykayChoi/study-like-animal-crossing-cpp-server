#include "world.h"
#include "actor/actor.h"

/////////////////////////////////////////////////////////////////////////////////////////
// World::TickThread

World::TickThread::TickThread(World& world)
    : Super("WorldTickThread")
    , world_(world)
{
}

void World::TickThread::Execute()
{
    auto lastTime = std::chrono::steady_clock::now();
    static const auto msPerTick = std::chrono::milliseconds(50);

    while (!bShouldTerminate_)
    {
        auto nowTime = std::chrono::steady_clock::now();
        auto delta
            = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastTime)
                  .count();
        world_.Tick(static_cast<float>(delta));
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
// World

World::World()
    : tickThread_(*this)
{
}

World::~World() { }

void World::Start() { tickThread_.Start(); }

void World::Stop() { tickThread_.Stop(); }

void World::AddActor(std::unique_ptr<Actor> actor)
{
    CSLock lock(csActors_);
    actors_.push_back(std::move(actor));
}

void World::AddClientHandler(ClientHandler* client)
{
    CSLock lock(csClients_);
    clientHandlers_.push_back(client);
}

void World::Tick(float delta)
{
    for (const auto& actor : actors_)
    {
        // TODO tick 필요한 액터 구분 필요.
        actor->Tick(delta);
    }
}

void World::BroadcastActorPosition(const Actor& actor, const ClientHandler* exclude)
{
    CSLock lock(csClients_);
    for (const ClientHandler* client : clientHandlers_)
    {
        if (Client == exclude)
        {
            continue;
        }

        // TODO

        std::vector<uint8> buf = BuildTownMovePacketBuffer(actor);
        client->SendActorPosition(buf);
    }
}

std::vector<uint8> World::BuildTownMovePacketBuffer(const Actor& actor)
{
    ByteBuffer buf(32); // HEADER(4) + INT(4) * type,x,y,z,degrees,spped(7) = 32
    uint8 firstByte = (uint8)((bufBodySize & 0xFF00) >> 8);
    uint8 secondByte = (uint8)(bufBodySize & 0x00FF);
    uint8 flag = (uint8)EPayloadFlag::Binary;
    uint8 zero = 0;
    buf.Write(&firstByte, 1);
    buf.Write(&secondByte, 1);
    buf.Write(&flag, 1);
    buf.Write(&zero, 1);

    buf.WriteInt32LE((int)EPacketType::TOWN_ACTOR_MOVE_SC);
    buf.WriteInt32LE(0); // TODO player인 경우 userId 넣어줘야 됨.

    Vector3<double> pos = actor.GetPosition();
    buf.WriteInt32LE(pos.x);
    buf.WriteInt32LE(pos.y);
    buf.WriteInt32LE(pos.z);
    // TODO degrees, spped
    buf.WriteInt32LE(0);
    buf.WriteInt32LE(0);

    return buf;
}