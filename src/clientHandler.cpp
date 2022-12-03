#include "clientHandler.h"
#include "packetHandler/packetHandler.h"
#include "json/json.h"
#include <chrono>

static const std::chrono::milliseconds PING_TIME_MS = std::chrono::milliseconds(1000);

int ClientHandler::clientCount_ = 0;

ClientHandler::ClientHandler(const std::string& ip)
    : ip_(ip)
    // TODO jaykay
    , bHasSentDC_(false)
    , ping_(1000)
    , pingId_(1)
    , connectionState_(EConnectionState::JustConnected)
    , uniqueId_(0)
    , bPrintThreadId_(false)
{
    packetHandler_ = std::make_unique<PacketHandler>(this);
    uniqueId_ = clientCount_++;
    pingStartTime_ = std::chrono::steady_clock::now();

    Log("New ClientHandler created at %p", static_cast<void*>(this));
}

ClientHandler::~ClientHandler()
{
    ASSERT(connectionState_ == EConnectionState::LoggedOut);

    Log("Deleting client \"%d\" at %p", id_, static_cast<void*>(this));

    Log("ClientHandler at %p deleted", static_cast<void*>(this));
}
