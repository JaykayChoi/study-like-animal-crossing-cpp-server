#pragma once

#include "../global.h"
#include <mutex>

class CriticalSection
{
public:
    void Lock();
    void Unlock();

    CriticalSection();

    /**
     * 현재 이 함수를 호출하는 스레드에 의해 잠겨있는지 반환.
     */
    bool IsLockedByCurThread();

private:
    /**
     * 잠긴 횟수.
     * std::atomic 을 사용할 경우 너무 많은 runtime penalty 가 발생할 수 있기 때문에
       std::atomic 을 사용하지 않음.
     */
    int recursionCount_;

    /**
     * 현재 CriticalSection 를 보유하고 있는 스레드의 id
     * 잠긴 경우에만 유효함. 언락되면 쓰레기값.
     */
    std::thread::id ownerThreadId_;

    std::recursive_mutex mutex_;
};

/**
 * RAII 패턴 CriticalSection.
 * 생성 시 잠기고 소멸 시 언락.
 */
class CSLock
{
    CriticalSection* cs_;

    /**
     * CriticalSection 과 달리 이 객체는 싱글 스레드에서 사용해야 되기 때문에
       bIsLocked_ 에 대한 접근은 thread-unsafe 하다.
     * 만약 잠금이 유지되지 않으면 CriticalSection::Unlock() 을 여러 번 호출하는 것은
       에러. 따라서 bIsLocked_ 을 통해 잠겨있는지 확인이 필요함.
     */
    bool bIsLocked_;

public:
    CSLock(CriticalSection* cs);
    CSLock(CriticalSection& cs);
    ~CSLock();

    void Lock();
    void Unlock();

private:
    DISALLOW_COPY_AND_ASSIGN(CSLock);
};

// CSLock 와 같이 사용되며 생성 시 언락되고 소멸 시 잠김.
class CSUnlock
{
    CSLock& lock_;

public:
    CSUnlock(CSLock& lock);
    ~CSUnlock();

private:
    DISALLOW_COPY_AND_ASSIGN(CSUnlock);
};
