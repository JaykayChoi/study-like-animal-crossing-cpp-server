#include "world.h"
#include "actor/actor.h"
#include "clientHandler.h"
#include "osLib/byteBuffer.h"
#include "packetHandler/packetHandler.h"
#include "util/vector3.h"

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
    for (ClientHandler* client : clientHandlers_)
    {
        if (client == exclude)
        {
            continue;
        }

        std::vector<uint8> buf = BuildTownMovePacketBuffer(actor);
        client->SendActorPosition(buf);
    }
}

std::vector<uint8> World::BuildTownMovePacketBuffer(const Actor& actor)
{
    int bodySize = 28;
    int headerSize = 4;
    std::vector<uint8> buf;
    buf.reserve(bodySize + headerSize);
    uint8 firstByte = (uint8)((bodySize & 0xFF00) >> 8);
    uint8 secondByte = (uint8)(bodySize & 0x00FF);
    buf[0] = firstByte;
    buf[1] = secondByte;
    buf[2] = (uint8)EPayloadFlag::Binary;
    buf[3] = 0;

    lutil::WriteInt32LEToUInt8Vector(buf, (int)EPacketType::TOWN_ACTOR_MOVE_SC);
    lutil::WriteInt32LEToUInt8Vector(buf, 0); // TODO id

    Vector3<double> pos = actor.GetPosition();
    lutil::WriteInt32LEToUInt8Vector(buf, pos.x);
    lutil::WriteInt32LEToUInt8Vector(buf, pos.y);
    lutil::WriteInt32LEToUInt8Vector(buf, pos.z);
    // TODO degrees, spped
    lutil::WriteInt32LEToUInt8Vector(buf, 0);
    lutil::WriteInt32LEToUInt8Vector(buf, 0);

    return buf;
}
