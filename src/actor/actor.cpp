#include "actor.h"
#include "../clientHandler.h"
#include "../world.h"

Actor::Actor()
    : world_(nullptr)
{
}

bool Actor::Initialize(std::unique_ptr<Actor> self, World& world)
{
    ASSERT(world_ == nullptr);
    SetWorld(&world);
    world.AddActor(std::move(self));

    return true;
}

void Actor::Tick(float delta)
{
    // TODO
}

void Actor::BroadcastMovement(const ClientHandler* exclude)
{
    world_->BroadcastActorPosition(*this, exclude);
}

void Actor::SetPosition(double inX, double inY, double inZ)
{
    lastPosition_ = position_;

    // TODO clamp

    position_ = { inX, inY, inZ };
}
