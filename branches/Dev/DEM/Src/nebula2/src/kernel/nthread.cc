//------------------------------------------------------------------------------
//  nthread.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "kernel/ntypes.h"
#include "kernel/nthread.h"

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
     - 27-Apr-99   floh    + support for __NEBULA_NO_THREADS__
     - 20-Feb-00   floh    + Win32: rewritten to _beginthreadx() instead
                             of _beginthread()
*/
nThread::nThread(int (N_THREADPROC* _thread_func)(nThread*),
                 Priority pri,
                 int stack_size,
                 void (*_wakeup_func)(nThread*),
                 nThreadSafeList* _ext_msglist,
                 void* _user_data)
{
#ifndef __NEBULA_NO_THREADS__
    n_assert(_thread_func);
    if (!stack_size)
    {
        stack_size = N_DEFAULT_STACKSIZE;
    }
    if (_ext_msglist)
    {
        this->msgList = _ext_msglist;
        this->isExtMsgList = true;
    }
    else
    {
        this->msgList = n_new(nThreadSafeList);
        this->isExtMsgList = false;
    }

    this->threadFunc = _thread_func;
    this->wakeupFunc = _wakeup_func;
    this->userData   = _user_data;
    this->stopThread = false;
    this->shutdownSignalReceived = false;

    // launch thread
#   ifdef __WIN32__
    // we are using _beginthreadx() instead of CreateThread(),
    // because we want to use c runtime functions from within the thread
    unsigned int thrdaddr;
    this->thread = (HANDLE)_beginthreadex(
                   NULL,    // security
                   stack_size,
                   (unsigned (__stdcall*)(void*))_thread_func,
                   this,    // arglist
                   0,       // init_flags
                   &thrdaddr);
    n_assert(this->thread);
    switch (pri)
    {
        case Low:
            SetThreadPriority(this->thread, THREAD_PRIORITY_BELOW_NORMAL);
            break;

        case Normal:
            SetThreadPriority(this->thread, THREAD_PRIORITY_NORMAL);
            break;

        case High:
            SetThreadPriority(this->thread, THREAD_PRIORITY_ABOVE_NORMAL);
            break;
    }

#   else
    // fix gcc warning
    pri = pri;

    // FIXME: ignore stack size under Linux
    int pok = pthread_create(&(this->thread),
                             NULL,
                             (void *(*)(void *))_thread_func,
                             this);
    n_assert(!((pok == EAGAIN) || (pok == EINVAL)))
#   endif

    // wait until the thread has started
    this->startupEvent.Wait();
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
     - 30-Oct-98   floh    Wenn sich der Thread lange vor
                           dem Aufruf des Destruktors selbst terminiert
                           hat, konnte es passieren, das WaitForSingleObject()
                           endlos haengen blieb, weil das Signal
                           offensichtlich schon lange verraucht war.
                           ThreadHarakiri() blockiert den Thread jetzt
                           solange, bis der Destruktor das shutdown_event
                           signalisiert
     - 31-Oct-98   floh    das shutdown_event konnte signalisiert werden,
                           bevor der Thread in ThreadHarakiri() darauf
                           warten konnte... deshalb setzt der Destruktor
                           jetzt das Signal in einer ausgebremsten Schleife
                           sooft, bis ThreadHarakiri() das Signal wirklich
                           empfangen konnte.
     - 26-Dec-98   floh    auf das shutdown-Signal vom Thread wird jetzt
                           nicht mehr in einer Schleife gewartet, weil
                           nEvent unter Linux jetzt auf Posix-Semaphoren
                           umgeschrieben wurde (welche hoffentlich
                           funktionieren).
     - 27-Apr-99   floh    + Support fuer __NEBULA_NO_THREADS__
     - 03-Feb-00   floh    + changed WaitForSingleObject() from
                             INFINITE to 1000 milliseconds
     - 20-Feb-00   floh    + Win32: rewritten to _beginthreadx(), _endthreadx()
     - 08-Nov-00   floh    + WaitForSingleObject() waits 500 milliseconds.
                             under WinNT/2000 with several socket threads
                             open, it can still happen that the socket
                             does not return
*/
nThread::~nThread()
{
#ifndef __NEBULA_NO_THREADS__
    // ask thread func to stop, if a wakeup func is defined
    // call it, so that the thread can be signaled to wake up
    // in order to know that it should terminate
    this->stopThread = true;
    if (this->wakeupFunc)
    {
        this->wakeupFunc(this);
    }

    // signal the thread that it may terminate now
    this->shutdownEvent.Signal();

    // wait until the thread has indeed terminated
    // (do nothing under Win32, because _endthreadex()
    // will be called at the end of the thread, which
    // does the CloseHandle() stuff itself
#   ifdef __WIN32__
    WaitForSingleObject(this->thread, 500);
    CloseHandle(this->thread);
    this->thread = 0;
#   else
    pthread_join(this->thread, NULL);
    this->thread = 0;
#   endif

    // flush msg list (all remaining messages will be lost)
    if (!(this->isExtMsgList))
    {
        nMsgNode* nd;
        this->msgList->Lock();
        while ((nd = (nMsgNode*)this->msgList->RemHead()))
        {
            n_delete(nd);
        }
        this->msgList->Unlock();
        n_delete(this->msgList);
    }
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
     - 30-Oct-98   floh    wartet jetzt auf den Destruktor
     - 27-Apr-99   floh    + Support fuer __NEBULA_NO_THREADS__
     - 20-Feb-00   floh    + rewritten to _endthreadex()
*/
void
nThread::ThreadHarakiri()
{
#ifndef __NEBULA_NO_THREADS__
    // synchronize with destructor
    this->shutdownEvent.Wait();
    this->shutdownSignalReceived = true;
#   ifdef __WIN32__
//    _endthreadex(0);
#   else
    pthread_exit(0);
#   endif
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
     - 27-Apr-99   floh    + Support fuer __NEBULA_NO_THREADS__
*/
void
nThread::ThreadStarted()
{
#ifndef __NEBULA_NO_THREADS__
    this->startupEvent.Signal();
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
     - 27-Apr-99   floh    + Support fuer __NEBULA_NO_THREADS__
*/
bool
nThread::ThreadStopRequested()
{
#ifndef __NEBULA_NO_THREADS__
    return this->stopThread;
#else
    return true;
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
void
nThread::ThreadSleep(float sec)
{
#ifndef __NEBULA_NO_THREADS__
    n_sleep(sec);
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
nMsgNode*
nThread::GetMsg()
{
#ifndef __NEBULA_NO_THREADS__
    nMsgNode* nd;
    this->msgList->Lock();
    nd = (nMsgNode*)this->msgList->RemHead();
    this->msgList->Unlock();
    return nd;
#else
    return NULL;
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
void
nThread::ReplyMsg(nMsgNode* nd)
{
#ifndef __NEBULA_NO_THREADS__
    n_delete(nd);
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
void
nThread::WaitMsg()
{
#ifndef __NEBULA_NO_THREADS__
    this->msgList->WaitEvent();
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
void
nThread::PutMsg(void* buf, int size)
{
#ifndef __NEBULA_NO_THREADS__
    n_assert(buf);
    n_assert(size > 0);
    nMsgNode* nd = n_new(nMsgNode(buf, size, 0));
    this->msgList->Lock();
    this->msgList->AddTail(nd);
    this->msgList->Unlock();
    this->msgList->SignalEvent();
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
void*
nThread::LockUserData()
{
#ifndef __NEBULA_NO_THREADS__
    if (this->userData)
    {
        this->userDataMutex.Lock();
    }
    return this->userData;
#else
    return 0;
#endif
}

//------------------------------------------------------------------------------
/**
     - 20-Oct-98   floh    created
*/
void
nThread::UnlockUserData()
{
#ifndef __NEBULA_NO_THREADS__
    this->userDataMutex.Unlock();
#endif
}

//------------------------------------------------------------------------------
//  EOF
//------------------------------------------------------------------------------
