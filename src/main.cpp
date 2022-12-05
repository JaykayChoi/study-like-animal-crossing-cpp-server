#include "global.h"
#include "osLib/networkManager.h"
#include "serverInstance.h"
#include "tclap/CmdLine.h"
// #include <windows.h>

void AbortSignalHandler(int signal);
static BOOL CtrlHandler(DWORD fdwCtrlType);
void ParseArguments(int argc, char** argv);
int Start();

int main(int argc, char** argv)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    std::signal(SIGSEGV, AbortSignalHandler);
    std::signal(SIGTERM, AbortSignalHandler);
    std::signal(SIGINT, AbortSignalHandler);
    std::signal(SIGABRT, AbortSignalHandler);
#ifdef SIGABRT_COMPAT
    std::signal(SIGABRT_COMPAT, AbortSignalHandler);
#endif
#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif

    VERIFY(SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(CtrlHandler), TRUE)
        == TRUE);

    ParseArguments(argc, argv);

    return Start();
}

void AbortSignalHandler(int signal)
{
    Log("Aborted from signal %d", signal);

    switch (signal)
    {
    case SIGSEGV:
        std::signal(SIGSEGV, SIG_DFL);
        return;
    case SIGABRT:
        std::signal(SIGABRT, SIG_DFL);
        return;
    case SIGINT:
    case SIGTERM:
        ServerInstance::Get().Stop();
        return;
    }
}

static BOOL CtrlHandler(DWORD fdwCtrlType)
{
    LogError("CtrlHandler");

    ServerInstance::Get().Stop();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    return TRUE;
}

void ParseArguments(int argc, char** argv)
{
    try
    {
        // TODO
        /*
        TCLAP::CmdLine cmd("Tutorial");
        TCLAP::ValueArg<int> tcPort("p", "port", "The port number the server should listen
        to", false, 23000, "port", cmd); cmd.parse(argc, argv);


        int temp = tcPort.getValue();
        LOG("port %d\n", temp);
        */
    }
    catch (const TCLAP::ArgException& exc)
    {
        LogError("Parse args is failed. line: %s, arg: %s", exc.error().c_str(),
            exc.argId().c_str());
    }
}

/*
NetworkManager::Initialize
ServerInstance::Run

Server::Init
Server::Start

Network::Listen

ListenServer::Listen

ListenServer::OnConnected

TCPConnection::Enable
TCPConnection::ReadCallback
TCPConnection::ReceivedCleartextData
ClientHandler::OnReceivedData
*/
int Start()
{
    struct NetworkRAII
    {
        NetworkRAII() { NetworkManager::Get().Initialize(); }

        ~NetworkRAII() { NetworkManager::Get().Terminate(); }
    };

    try
    {
        while (true)
        {
            NetworkRAII raii;

            if (!ServerInstance::Get().Run())
            {
                break;
            }
        }

        return EXIT_SUCCESS;
    }
    catch (const fmt::format_error& err)
    {
        std::cerr << "Main() exception: " << err.what() << '\n';
    }
    catch (const TCLAP::ArgException& err)
    {
        std::cerr << fmt::sprintf(
            "Error reading command line {} for argument {}\n", err.error(), err.argId());
    }
    catch (const std::exception& err)
    {
        std::cerr << "Main() exception: " << err.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception\n";
    }

    return EXIT_FAILURE;
}