#include "serverInstance.h"
#include "sql/authDb.h"
#include "sql/userDb.h"

ServerInstance::ServerInstance()
    : server_(nullptr)
    , authDb_(nullptr)
    , userDb_(nullptr)
{
    ChangeState(ENextState::Run);
}

ServerInstance::~ServerInstance() { }

ServerInstance& ServerInstance::Get()
{
    static ServerInstance instance;
    return instance;
}

bool ServerInstance::Start()
{
    auto beginTime = std::chrono::steady_clock::now();
    Log("Creating new server instance...");
    server_ = new Server();

    Log("Initializing db connection pool...");

#ifdef USE_RDB
    // TODO config
    std::map<mysql_option, std::string> options;
    options.insert(
        std::pair<mysql_option, std::string>(MYSQL_OPT_CONNECT_TIMEOUT, "3000"));
    authDb_ = new AuthDb(2, "localhost", "root", "dev123$", "lac_auth", 3306,
        CLIENT_MULTI_STATEMENTS | CLIENT_FOUND_ROWS, options);

    userDb_ = new UserDb(2, 1, "localhost", "root", "dev123$", "lac_user", 3306,
        CLIENT_MULTI_STATEMENTS | CLIENT_FOUND_ROWS, options);
#else
/**
 * TODO db interface 를 만들어서 userdb, authdb 대신할 개발용 디비를(메모리 or sqllite)
 * 만든다.
 */
#endif

    Log("Initializing server...");

    if (!server_->Init())
    {
        throw std::runtime_error("Failed to start server.");
    }

    Log("Starting server...");

    if (server_->Start())
    {
        Log("Server is started.");
        startTime_ = std::chrono::steady_clock::now();

        stopEvent_.Wait();

        Log("Shutting down server....");
        server_->Shutdown();
    }

    Log("Cleaning up server...");
    delete server_;
    server_ = nullptr;

    Log("Shutdown successful.");

    return curState_ == ENextState::Restart;
}

void ServerInstance::Stop() { ChangeState(ENextState::Stop); }

void ServerInstance::Restart() { ChangeState(ENextState::Restart); }

void ServerInstance::ChangeState(ENextState newState)
{
    if (curState_ == ENextState::Stop)
    {
        return;
    }

    curState_ = newState;

    if (curState_ == ENextState::Run)
    {
        return;
    }

    stopEvent_.Set();
}