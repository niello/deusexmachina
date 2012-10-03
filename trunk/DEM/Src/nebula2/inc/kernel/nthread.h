#ifndef N_THREAD_H
#define N_THREAD_H
//------------------------------------------------------------------------------
/**
    @class nThread
    @ingroup Threading
    @brief A thread wrapper class for Nebula.

    Wraps an user defined thread function into a c++ object,
    additionally offers a message list for safe communication
    with the thread function.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "kernel/nmutex.h"
#include "kernel/nevent.h"
#include "kernel/nthreadsafelist.h"
#include "util/nmsgnode.h"

//------------------------------------------------------------------------------
#ifdef __WIN32__
#define N_THREADPROC __stdcall
#else
#define N_THREADPROC
#endif

//------------------------------------------------------------------------------
class nThread
{
public:
    /// thread priority levels
    enum Priority
    {
        Low,
        Normal,
        High
    };

    /// constructor
    nThread(int (N_THREADPROC* _thread_func)(nThread*),
            Priority pri,
            int stack_size,
            void (*_wakeup_func)(nThread*),
            nThreadSafeList* _ext_msglist,
            void* _user_data);
    /// destructor
    ~nThread();
    /// called by thread func at startup
    void ThreadStarted();
    /// called by thread func to determine when it should terminate
    bool ThreadStopRequested();
    /// called by thread func if it wants to sleep
    void ThreadSleep(float sec);
    /// must be called by thread func right before termination
    void ThreadHarakiri();
    /// wait for next message
    void WaitMsg();
    /// remove first message from message list
    nMsgNode* GetMsg();
    /// tell message list that we are done with the msg
    void ReplyMsg(nMsgNode*);
    /// put a message on the message list
    void PutMsg(void* buf, int bufSize);
    /// lock user data field
    void* LockUserData();
    /// unlock user data field
    void UnlockUserData();

private:
#ifndef __NEBULA_NO_THREADS__
    nMutex userDataMutex;
    nEvent sleepEvent;
    nEvent shutdownSleepEvent;
    nEvent shutdownEvent;
    nEvent startupEvent;
    bool stopThread;
    bool shutdownSignalReceived;
    int (N_THREADPROC *threadFunc)(nThread*);
    void (*wakeupFunc)(nThread*);

    nThreadSafeList* msgList;
    bool isExtMsgList;
    void* userData;

#   ifdef __WIN32__
    HANDLE thread;
#   else
    pthread_t thread;
#   endif
#endif

    enum
    {
        N_DEFAULT_STACKSIZE = 4096,
    };
};

//-------------------------------------------------------------------
#endif
