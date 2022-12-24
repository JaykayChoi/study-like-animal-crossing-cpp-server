#include "thread.h"
#include <windows.h>

// https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
#if defined(_DEBUG)
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
struct THREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1 = caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
};
#pragma pack(pop)
static void SetThreadName(std::thread* a_Thread, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = GetThreadId(a_Thread->native_handle());
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
    __try
    {
        RaiseException(
            MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#pragma warning(pop)
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// Thread

Thread::Thread(const std::string& threadName)
    : bShouldTerminate_(false)
    , threadName_(threadName)
{
}

Thread::~Thread() { Stop(); }

bool Thread::Start()
{
    try
    {
        // 스레드 초기화.
        thread_ = std::thread(&Thread::DoExecute, this);

#ifdef _DEBUG
        if (!threadName_.empty())
        {
            SetThreadName(&thread_, threadName_.c_str());
        }
#endif

        // 스레드 시작이 완료되었다는 것을 노티.
        eventStart_.Notify();

        return true;
    }
    catch (const std::system_error& err)
    {
        LogError(
            "Thread::Start error occured. Error code: %i, thread name: %s, error msg: %s",
            err.code().value(), threadName_.c_str(), err.code().message().c_str());
        return false;
    }
}

void Thread::Stop()
{
    bShouldTerminate_ = true;
    Wait();
    bShouldTerminate_ = false;
}

bool Thread::Wait()
{
    Log("Waiting for thread %s to finish.", threadName_.c_str());
    if (thread_.joinable())
    {
        try
        {
            thread_.join();
            return true;
        }
        catch (const std::system_error& err)
        {
            LogError("%s error occured. Error code: %i, thread name: %s, error msg: %s",
                __FUNCTION__, err.code().value(), threadName_.c_str(),
                err.code().message().c_str());
            return false;
        }
    }

    Log("Thread %s finished.", threadName_.c_str());
    return true;
}

bool Thread::IsCurrentThread() const
{
    return std::this_thread::get_id() == thread_.get_id();
}

void Thread::DoExecute()
{
    eventStart_.Wait();
    Execute();
}
