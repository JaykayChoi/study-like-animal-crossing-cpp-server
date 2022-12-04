#pragma once

#include "../global.h"
#include "criticalSection.h"
#include "network.h"
#include "wEvent.h"
#include <event2/event.h>

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager() noexcept(false); // ASSERT 가 있음.

    static NetworkManager& Get();

    // 모든 네트워크 관련 쓰레드 초기화.
    void Initialize();

    // 모든 네트워크 관련 쓰레드 종료.
    void Terminate();

    // 이벤트 등록을 위한 libevent handle 반환
    event_base* GetEventBase() { return eventBase_; }

    /**
     * listenServers_ 에 server 를 추가한다.
     * 새 수신 서버가 작성 될 때 기본 서버 핸들 구현에서 사용된다.
     * 리스닝에 성공한 서버 만 추가된다.
	 * TODO IListenServer * or & 으로 받는 것이 나을지 고민.
	 * https://www.modernescpp.com/index.php/c-core-guidelines-passing-smart-pointer
	 * https://stackoverflow.com/questions/3310737/should-we-pass-a-shared-ptr-by-reference-or-by-value
     */
    void AddListenServer(const std::shared_ptr<IListenServer>& server);

    void RemoveListenServer(const IListenServer* server);

protected:
    // event loop 를 구동하기 위한 기본 libevent 컨테이너.
    event_base* eventBase_;

    // 현재 활성 상태인 리슨 서버들.
    std::vector<std::shared_ptr<IListenServer>> listenServers_;

    // Protect all containers.
    CriticalSection cs_;

    // Terminate() 가 호출된 경우 true 로 설정.
    std::atomic<bool> bHasTerminated_;

    // 기본 libevent 루프가 실행되는 스레드.
    std::thread eventLoopThread_;

    // 시작이 완료되고 libevent 루프가 실행되면 신호를 보내는 이벤트.
    WEvent startupEvent_;

    static void LogCallback(int severity, const char* msg);

    /**
     * http://wiki.pchero21.com/wiki/Libevent_R3:_Working_with_an_event_loop
     * eventBase_ 의 event dispatcher loop 를 실행하는 스레드를 구현.
     */
    static void RunEventLoop(NetworkManager* self);

    // 이벤트 루프가 시작될 때 libevent 에 의해 호출되는 콜백.
    static void EventLoopCallback(evutil_socket_t socket, short events, void* inSelf);
};
