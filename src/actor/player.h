#pragma once

#include "../global.h"
#include "pawn.h"

class ClientHandler;

class Player : public Pawn
{
    using Super = Pawn;

public:
    /**
     * TODO ClientHandler * or & 으로 받는 것이 나을지 고민.
     * https://www.modernescpp.com/index.php/c-core-guidelines-passing-smart-pointer
     * https://stackoverflow.com/questions/3310737/should-we-pass-a-shared-ptr-by-reference-or-by-value
     */
    Player(const std::shared_ptr<ClientHandler>& client);

    virtual ~Player() override;

    // Actor overrides.
    virtual void Tick(float delta) override;

private:
    std::shared_ptr<ClientHandler> clientHandler_;

    virtual void BroadcastMovement(const ClientHandler* exclude = nullptr) override;
};