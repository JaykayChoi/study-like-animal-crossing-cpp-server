#pragma once

#include "../global.h"

class World;

class Actor
{
public:
    Actor();
    virtual ~Actor() = default;

    bool Initialize(std::unique_ptr<Actor> self, World& world);

    void SetWorld(World* world) { world_ = world; }

    virtual void Tick(float delta);

private:
    World* world_;
};