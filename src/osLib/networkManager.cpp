#include "networkManager.h"
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/thread.h>

NetworkManager::NetworkManager()
    : bHasTerminated_(true)
{
}

NetworkManager::~NetworkManager() noexcept(false)
{
    // Terminate() 가 호출되었는지 확인.
    ASSERT(bHasTerminated_);
}

NetworkManager& NetworkManager::Get()
{
    static NetworkManager instance;
    return instance;
}

void NetworkManager::Initialize()
{
    // 윈속 초기화
    WSADATA wsaData;
    memset(&wsaData, 0, sizeof(wsaData));
    // 2.2 버전의 윈속 사용
    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0)
    {
        int err = WSAGetLastError();
        LogWarn("WSAStartup failed: %d, WSAGLE: %d (%s)", res, err,
            evutil_socket_error_to_string(err));
        exit(1);
    }

    /**
     * http://wiki.pchero21.com/wiki/Libevent_R1:_Setting_up_the_Libevent_library
     * 로깅 함수 오버라이드.
     * libevent 가 로그를 기록할 때 LogCallback 호출 됨.
     */
    event_set_log_callback(LogCallback);

    /**
     * Windows threading 방식을 사용.
     * threading 초기화.
     */
    evthread_use_windows_threads();

    /**
     * http://wiki.pchero21.com/wiki/Libevent_R2:_Getting_an_event_base
     * libevent 를 사용하기 전 event_base_structure 생성 필요함.
     * event_base_structure 는 설정된 이벤트를 감시하며, Poll 한다.
     * event_base 생성
     */
    event_config* config = event_config_new();
    event_config_set_flag(config, EVENT_BASE_FLAG_STARTUP_IOCP); // IOCP 사용
    eventBase_ = event_base_new_with_config(config);
    if (eventBase_ == nullptr)
    {
        LogError("Failed to initialize event base.");
        abort();
    }
    event_config_free(config);

    // Event loop thread 생성.
    bHasTerminated_ = false;
    eventLoopThread_ = std::thread(RunEventLoop, this);
    // Libevent 루프가 실제로 실행될 때까지 기다린다.
    startupEvent_.Wait();
}

void NetworkManager::Terminate()
{
    ASSERT(!bHasTerminated_);

    // Libevent event loop 가 종료될 때까지 기다린다.
    event_base_loopbreak(eventBase_);
    eventLoopThread_.join();

    // Close all open connections.
    {
        CSLock lock(cs_);
        auto servers = listenServers_;
        for (auto& server : servers)
        {
            server->Close();
        }
        ASSERT(listenServers_.empty());
    }

    // 기본 libevnet 객체를 해제.
    event_base_free(eventBase_);

    // libevent 는 main() 함수가 종료되기전에 셧타운 되어야 된다.
    libevent_global_shutdown();

    bHasTerminated_ = true;
}

void NetworkManager::AddListenServer(const std::shared_ptr<IListenServer>& server)
{
    ASSERT(!bHasTerminated_);
    CSLock lock(cs_);
    listenServers_.push_back(server);
}

void NetworkManager::RemoveListenServer(const IListenServer* server)
{
    ASSERT(!bHasTerminated_);
    CSLock lock(cs_);
    for (auto itr = listenServers_.begin(); itr != listenServers_.end(); itr++)
    {
        if (itr->get() == server)
        {
            listenServers_.erase(itr);
            return;
        }
    }
}

void NetworkManager::LogCallback(int severity, const char* msg)
{
    switch (severity)
    {
    case _EVENT_LOG_DEBUG:
        LogError("LibEvent: %s", msg);
        break;
    case _EVENT_LOG_MSG:
        Log("LibEvent: %s", msg);
        break;
    case _EVENT_LOG_WARN:
        LogWarn("LibEvent: %s", msg);
        break;
    case _EVENT_LOG_ERR:
        LogError("LibEvent: %s", msg);
        break;
    default:
        LogWarn("LibEvent: Unknown log severity (%d): %s", severity, msg);
        break;
    }
}

void NetworkManager::RunEventLoop(NetworkManager* self)
{
    auto timer = evtimer_new(self->eventBase_, EventLoopCallback, self);
    timeval timeout {}; // Zero timeout.
    evtimer_add(timer, &timeout);
    event_base_loop(self->eventBase_, EVLOOP_NO_EXIT_ON_EMPTY);
    event_free(timer);
}

void NetworkManager::EventLoopCallback(evutil_socket_t socket, short events, void* inSelf)
{
    auto self = static_cast<NetworkManager*>(inSelf);
    ASSERT(self != nullptr);
    self->startupEvent_.Set();
}
