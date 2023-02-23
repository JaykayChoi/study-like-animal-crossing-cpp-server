#pragma once

#include "../global.h"
#include "../util/vector3.h"

class World;
class ClientHandler;

class Actor
{
public:
    Actor();
    virtual ~Actor() = default;

    bool Initialize(std::unique_ptr<Actor> self, World& world);

    void SetWorld(World* world) { world_ = world; }

    virtual void Tick(float delta);

    virtual void BroadcastMovement(const ClientHandler* exclude = nullptr);

    const Vector3<double>& GetPosition() const { return position_; }

    void SetPosition(double inX, double inY, double inZ);

    void SetPosX(double inX) { SetPosition(inX, position_.y, position_.z); }
    void SetPosY(double inY) { SetPosition(position_.x, inY, position_.z); }
    void SetPosZ(double inZ) { SetPosition(position_.x, position_.y, inZ); }

private:
    World* world_;

    Vector3<double> position_;
    Vector3<double> lastPosition_;
};