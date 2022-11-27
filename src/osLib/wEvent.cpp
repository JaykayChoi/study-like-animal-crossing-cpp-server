#include "wEvent.h"

WEvent::WEvent()
    : bCanContinue_(false)
{
}

void WEvent::Wait()
{
    std::unique_lock<std::mutex> lock(mutex_);

    // condVar_ 에서 notify 를 받을 때까지 현재의 스레드를 중단한다.
    condVar_.wait(lock, [this]() { return bCanContinue_; });
    bCanContinue_ = false;
}

bool WEvent::Wait(uint32 timeoutMSec)
{
    auto dst = std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMSec);
    bool result;
    {
        std::unique_lock<std::mutex> lock(mutex_);

        /**
         * condVar_ 에서 notify 를 받을 때까지 현재의 스레드를 중단한다.
         * dst 이 지날 때까지 통지가 없다면 timeout을 발생시킨다.
         */
        result = condVar_.wait_until(lock, dst, [this]() { return bCanContinue_; });
        bCanContinue_ = false;
    }
    return result;
}

void WEvent::Set()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        bCanContinue_ = true;
    }

    // 대기하고 있는 하나의 스레드를 깨운다.
    condVar_.notify_one();
}

void WEvent::SetAll()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        bCanContinue_ = true;
    }

    // 대기하고 있는 모든 스레드를 깨운다.
    condVar_.notify_all();
}
