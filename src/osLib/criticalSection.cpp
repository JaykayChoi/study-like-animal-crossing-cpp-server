#include "criticalSection.h"

/////////////////////////////////////////////////////////////////////////////////////////
// CriticalSection

CriticalSection::CriticalSection()
    : recursionCount_(0)
{
}

void CriticalSection::Lock()
{
    mutex_.lock();

    recursionCount_++;
    ownerThreadId_ = std::this_thread::get_id();
}

void CriticalSection::Unlock()
{
    ASSERT(IsLockedByCurThread());
    recursionCount_--;

    mutex_.unlock();
}

bool CriticalSection::IsLockedByCurThread()
{
    return ((recursionCount_ > 0) && (ownerThreadId_ == std::this_thread::get_id()));
}

/////////////////////////////////////////////////////////////////////////////////////////
// CSLock

CSLock::CSLock(CriticalSection* cs)
    : cs_(cs)
    , bIsLocked_(false)
{
    Lock();
}

CSLock::CSLock(CriticalSection& cs)
    : cs_(&cs)
    , bIsLocked_(false)
{
    Lock();
}

CSLock::~CSLock()
{
    if (!bIsLocked_)
    {
        return;
    }
    Unlock();
}

void CSLock::Lock()
{
    ASSERT(!bIsLocked_);
    bIsLocked_ = true;
    cs_->Lock();
}

void CSLock::Unlock()
{
    ASSERT(bIsLocked_);
    bIsLocked_ = false;
    cs_->Unlock();
}

/////////////////////////////////////////////////////////////////////////////////////////
// CSUnlock

CSUnlock::CSUnlock(CSLock& lock)
    : lock_(lock)
{
    lock_.Unlock();
}

CSUnlock::~CSUnlock() { lock_.Lock(); }
