#ifndef N_RESOURCESERVER_H
#define N_RESOURCESERVER_H
//------------------------------------------------------------------------------
/**
    @class nResourceServer
    @ingroup Resource
    @brief Central resource server. Creates and manages resource objects.

    Resources are objects which provide several types of data (or data
    streams) to the application, and can unload and reload themselves
    on request.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/nroot.h"
#include "resource/nresource.h"
#include "kernel/nref.h"
#include <Data/Singleton.h>

//------------------------------------------------------------------------------
class nResourceServer: public nReferenced
{
	__DeclareSingleton(nResourceServer);

public:

	nResourceServer();
    virtual ~nResourceServer();

    virtual nResource* FindResource(const nString& rsrcName, nResource::Type rsrcType);
    virtual nResource* NewResource(const nString& className, const nString& rsrcName, nResource::Type rsrcType);

	virtual bool LoadResources(int resTypeMask);
    virtual void UnloadResources(int resTypeMask);
    virtual void OnLost(int resTypeMask);
    virtual void OnRestored(int resTypeMask);

	nRoot* GetResourcePool(nResource::Type rsrcType);

protected:
    friend class nResource;

    nString GetResourceId(const nString& rsrcName);
    /// add a resource to the loader job list
    void AddLoaderJob(nResource* res);
    /// remove a resource from the loader job list
    void RemLoaderJob(nResource* res);
    /// start the loader thread
    void StartLoaderThread();
    /// shutdown the loader thread
    void ShutdownLoaderThread();
    /// the loader thread function
    static int N_THREADPROC LoaderThreadFunc(nThread* thread);
    /// the thread wakeup function
    static void ThreadWakeupFunc(nThread* thread);

    int uniqueId;
    nRef<nRoot> meshPool;
    nRef<nRoot> texPool;
    nRef<nRoot> shdPool;
    nRef<nRoot> animPool;
    nRef<nRoot> sndResPool;
    nRef<nRoot> otherPool;

    nThreadSafeList jobList;    // list for outstanding background loader jobs
    nThread* loaderThread;      // background thread for handling async resource loading

    nClass* resourceClass;
};

//------------------------------------------------------------------------------
#endif
