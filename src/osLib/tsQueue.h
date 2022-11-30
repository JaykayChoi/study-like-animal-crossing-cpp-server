#pragma once

#include "../global.h"
#include "Thread.h"
#include "criticalSection.h"
#include "wEvent.h"

template <class T> struct QueueFuncs
{
public:
    // 삽입되는 항목이 반환되지 않고 queue 에서 삭제될 때 호출 됨.
    static void Delete(T) { }

    // EnqueueIfNotPresent 으로 insert 될 때 이미 동일한 값이 queue에 있을 경우 호출됨.
    static void Combine(T& existing, const T& newItem)
    {
        // Do nothing.
    }
};

// Thread safe queue.
template <class ItemType, class Funcs = QueueFuncs<ItemType>> class TSQueue
{
    typedef typename std::list<ItemType> QueueType;
    typedef typename QueueType::iterator iterator;

public:
    TSQueue() { }
    ~TSQueue() { }

    // 다른 스레드가 queue 에 접근하는 경우 차단될 수 있음.
    void Enqueue(ItemType item)
    {
        CSLock lock(cs_);
        contents_.push_back(item);
        eventAdded_.Set();
    }

    /**
     * 중복 없을 경우 삽입.
     * 다른 스레드가 queue 에 접근하는 경우 차단될 수 있음.
     */
    void EnqueueIfNotPresent(ItemType item)
    {
        CSLock lock(cs_);

        for (iterator itr = contents_.begin(); itr != contents_.end(); itr++)
        {
            if (*itr == item)
            {
                Funcs::Combine(*itr, item);
                return;
            }
        }

        contents_.push_back(item);
        eventAdded_.Set();
    }

    /**
     * 값이 존재할 경우 dequeue
     * 성공할 경우 true 반환.
     */
    bool TryDequeue(ItemType& item)
    {
        CSLock lock(cs_);
        if (contents_.empty())
        {
            return false;
        }
        item = contents_.front();
        contents_.pop_front();
        eventRemoved_.Set();
        return true;
    }

    // 값이 존재하지 않을 경우 값이 삽입될 때까지 락.
    ItemType Dequeue()
    {
        CSLock lock(cs_);
        while (contents_.empty())
        {
            CSUnlock unlock(lock);
            eventAdded_.Wait();
        }
        ItemType item = contents_.front();
        contents_.pop_front();
        eventRemoved_.Set();
        return item;
    }

    // Queue 가 비어질 때까지 락.
    void BlockUntillEmpty()
    {
        CSLock lock(cs_);
        while (!contents_.empty())
        {
            CSUnlock unlock(lock);
            eventRemoved_.Wait();
        }
    }

    void Clear()
    {
        CSLock lock(cs_);
        while (!contents_.empty())
        {
            Funcs::Delete(contents_.front());
            contents_.pop_front();
        }
    }

    /**
     * 호출되는 시점의 size 반환.
     * DequeueItem() 을 사용해도 되는지 결정할 때 사용하지 말 것.
     * 대신 TryDequeueItem() 사용해야 됨.
     */
    size_t Size()
    {
        CSLock lock(cs_);
        return contents_.size();
    }

    /**
     * 항목 하나 제거.
     * 제거할 항목이 존재했을 경우 true 반환.
     */
    bool Remove(ItemType item)
    {
        CSLock lock(cs_);
        for (iterator itr = contents_.begin(); itr != contents_.end(); itr++)
        {
            if ((*itr) == item)
            {
                contents_.erase(itr);
                eventRemoved_.Set();
                return true;
            }
        }
        return false;
    }

    // @param predicate 가 true 를 반환하는 모든 항목 제거
    template <class Predicate> void RemoveIfTrue(Predicate predicate)
    {
        CSLock lock(cs_);
        for (auto itr = contents_.begin(); itr != contents_.end();)
        {
            if (predicate(*itr))
            {
                auto itr2 = itr;
                itr2++;
                contents_.erase(itr);
                eventRemoved_.Set();
                itr = itr2;
            }
            else
            {
                itr++;
            }
        }
    }

private:
    // Queue
    QueueType contents_;

    // m_Contents 의 접근을 보호.
    CriticalSection cs_;

    // 항목이 삽입될 때 event 가 발생하는 WEvent
    WEvent eventAdded_;

    // 항목이 제거될 때 event 가 발생하는 WEvent
    WEvent eventRemoved_;
};
