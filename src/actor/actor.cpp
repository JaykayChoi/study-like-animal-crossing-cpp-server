﻿#include "actor.h"
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