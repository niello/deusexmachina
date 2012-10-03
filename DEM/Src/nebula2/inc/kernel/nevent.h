#ifndef N_EVENT_H
#define N_EVENT_H
//------------------------------------------------------------------------------
/**
    @class nEvent
    @ingroup Threading

    @brief Event wrapper for multithreading synchronization.

    THERE MAY BE DIFFERENCES IN BEHAVIOUR IF AN EVENT IS SIGNALLED
    WITHOUT ANY THREADS WAITING FOR IT. THE EVENT MAY OR MAY NOT
    REMAIN SIGNALLED BASED ON THE PLATFORM.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

#ifndef __NEBULA_NO_THREADS__
#   ifdef __WIN32__
#       ifndef _INC_WINDOWS
#       define WIN32_LEAN_AND_MEAN
#       include <windows.h>
#       endif
#       ifndef _INC_PROCESS
#       include <process.h>
#       endif
#   else
#       include <semaphore.h>
#       include <sys/time.h>
#       include <unistd.h>
#   endif
#endif

//------------------------------------------------------------------------------
class nEvent
{
public:
    /// constructor
    nEvent();
    /// destructor
    ~nEvent();
    /// signal the event
    void Signal();
    /// put event into wait state
    void Wait();
    /// wait state with timeout
    bool TimedWait(int ms);

private:

#ifndef __NEBULA_NO_THREADS__
    #ifdef __WIN32__
        HANDLE wevent;
    #else
        sem_t sem;
    #endif
#endif
};

//------------------------------------------------------------------------------
/**
*/
inline
nEvent::nEvent()
{
#ifndef __NEBULA_NO_THREADS__
    #ifdef __WIN32__
        this->wevent = CreateEvent(NULL, FALSE, FALSE, NULL);
        n_assert(this->wevent);
    #else
        sem_init(&(this->sem), 0, 0);
    #endif
#endif
}

//------------------------------------------------------------------------------
/**
*/
inline
nEvent::~nEvent()
{
#ifndef __NEBULA_NO_THREADS__
    #ifdef __WIN32__
        CloseHandle(this->wevent);
    #else
        sem_destroy(&(this->sem));
#   endif
#endif
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nEvent::Signal()
{
#ifndef __NEBULA_NO_THREADS__
    #ifdef __WIN32__
        SetEvent(this->wevent);
    #else
        sem_post(&(this->sem));
    #endif
#endif
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nEvent::Wait()
{
#ifndef __NEBULA_NO_THREADS__
    #ifdef __WIN32__
        WaitForSingleObject(this->wevent, INFINITE);
    #else
        sem_wait(&(this->sem));
    #endif
#endif
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nEvent::TimedWait(int ms)
{
#ifndef __NEBULA_NO_THREADS__
    #ifdef __WIN32__
        int r = WaitForSingleObject(this->wevent, ms);
        return (WAIT_TIMEOUT == r) ? false : true;
    #else
        // HACK
        while (ms > 0)
        {
            if (0 == sem_trywait(&(this->sem)))
            {
                return true;
            }
            usleep(1000);
            ms -= 1;
        }
        return false;
    #endif
#else
    return false;
#endif
}

//--------------------------------------------------------------------
#endif
