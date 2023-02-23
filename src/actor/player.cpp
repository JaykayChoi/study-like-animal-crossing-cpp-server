#include "player.h"
#include "../clientHandler.h"

Player::Player(const std::shared_ptr<ClientHandler>& client)
    : clientHandler_(client)
{
}

Player::~Player()
{
    // TODO
}

void Player::Tick(float delta)
{
    clientHandler_->WorldTick(delta);

    BroadcastMovement(clientHandler_.get());
}

void Player::BroadcastMovement(const ClientHandler* exclude)
{
    // TODO player 특성 처리.

    Super::BroadcastMovement(exclude);
}