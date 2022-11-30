#pragma once

#include "../global.h"
#include <condition_variable>
#include <mutex>

// Waitalbe event.
class WEvent
{
public:
    WEvent();

    // 이벤트가 Set 되기 전까지 기다린다. 이미 Set 되어 있을 경우 즉시 반환된다.
    void Wait();

    /**
     * 이벤트가 Set 되기 전까지 기다리거나 timeout 이 될 때까지 기다린다.
     * Set 된 경우에만 true를 반환하고, timeout 이 되었거나 에러가 발생할 경우 false 를
       반환한다.
     */
    bool Wait(uint32 timeoutMSec);

    /**
     * 이벤트를 Set 하고 Wait 에서 대기하고 있는 하나의 스레드를 릴리즈한다.
     * 대기중인 스레드가 없을 경우 다음번에 호출되는 Wait() 가 블럭되지 않는다.
     */
    void Set();

    /**
     * Wait 중인 모든 스레드를 Set 하여 릴리즈한다.
     * 대기중인 스레드가 없을 경우 다음번에 호출되는 Wait() 가 블럭되지 않는다.
     */
    void SetAll();

private:
    // spurious wakeups 을 방지하는데 사용된다.
    bool bCanContinue_;

    std::mutex mutex_;
    std::condition_variable condVar_;
};