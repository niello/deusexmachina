#ifndef N_THREADSAFEARRAY_H
#define N_THREADSAFEARRAY_H
//------------------------------------------------------------------------------
/**
    @class nThreadSafeArray
    @ingroup Threading
    @brief A thread safe array dynamic array template class.

    Offers method to manipulate dynamic arrays in a thread safe way,
    so that the array can be used as a communication point between
    threads.

    (C) 2002 RadonLabs GmbH
*/
#include "util/narray.h"
#include "kernel/nmutex.h"
#include "kernel/nevent.h"

//------------------------------------------------------------------------------
template<class TYPE>
class nThreadSafeArray : public nArray<TYPE>
{
public:
    /// signal event object
    void SignalEvent();
    /// wait for event to become signaled
    void WaitEvent();
    /// wait for event to become signaled with timeout
    void TimedWaitEvent(int ms);
    /// gain access to list
    void Lock();
    /// give up access to list
    void Unlock();

private:
    nMutex mutex;
    nEvent event;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadSafeArray<TYPE>::SignalEvent()
{
    this->event.Signal();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadSafeArray<TYPE>::WaitEvent()
{
    this->event.Wait();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadSafeArray<TYPE>::TimedWaitEvent(int ms)
{
    this->event.TimedWait(ms);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadSafeArray<TYPE>::Lock()
{
    this->mutex.Lock();
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
void
nThreadSafeArray<TYPE>::Unlock()
{
    this->mutex.Unlock();
}

//------------------------------------------------------------------------------
#endif
