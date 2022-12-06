#pragma once

#include "global.h"
#include "server.h"

class AuthDb;
class UserDb;

class ServerInstance
{
public:
    ServerInstance();
    ~ServerInstance();

    static ServerInstance& Get();

    bool Run();

    void Stop();

    void Restart();

    Server* GetServer() { return server_; }

    AuthDb* GetAuthDb() { return authDb_; }
    UserDb* GetUserDb() { return userDb_; }

private:
    enum class ENextState
    {
        Run,
        Restart,
        Stop
    };

    Server* server_;

    AuthDb* authDb_;
    UserDb* userDb_;

    ENextState curState_;

    WEvent stopEvent_;

    std::chrono::steady_clock::time_point startTime_;

    void ChangeState(ENextState newState);
};