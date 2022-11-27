#include "global.h"
#include <windows.h>

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
        // TODO
        return;
    }
}

static BOOL CtrlHandler(DWORD fdwCtrlType)
{
    LogError("CtrlHandler");
    // TODO

    return TRUE;
}

void ParseArguments(int argc, char** argv)
{
    // TODO
}

int Start()
{
    // TODO

    return EXIT_FAILURE;
}