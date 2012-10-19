//------------------------------------------------------------------------------
//  nresourceserver_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "resource/nresourceserver.h"
#include "resource/nresource.h"
#include <kernel/nkernelserver.h>

__ImplementSingleton(nResourceServer);

//------------------------------------------------------------------------------
/**
*/
nResourceServer::nResourceServer() :
    uniqueId(0),
    loaderThread(0)
{
    __ConstructSingleton;

    this->meshPool    = nKernelServer::Instance()->New("nroot", "/sys/share/rsrc/mesh");
    this->texPool     = nKernelServer::Instance()->New("nroot", "/sys/share/rsrc/tex");
    this->shdPool     = nKernelServer::Instance()->New("nroot", "/sys/share/rsrc/shd");
    this->animPool    = nKernelServer::Instance()->New("nroot", "/sys/share/rsrc/anim");
    this->sndResPool  = nKernelServer::Instance()->New("nroot", "/sys/share/rsrc/sndrsrc");
    this->otherPool   = nKernelServer::Instance()->New("nroot", "/sys/share/rsrc/other");

    this->resourceClass = nKernelServer::Instance()->FindClass("nresource");
    n_assert(this->resourceClass);

    #ifndef __NEBULA_NO_THREADS__
    this->StartLoaderThread();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
nResourceServer::~nResourceServer()
{
    #ifndef __NEBULA_NO_THREADS__
    this->ShutdownLoaderThread();
    #endif
    this->UnloadResources(nResource::AllResourceTypes);

	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Create a resource id from a resource name. The resource name is usually
    just the filename of the resource file. The method strips off the last
    32 characters from the resource name, and replaces any invalid characters
    with underscores. It is valid to provide a 0-rsrcName for unshared resources.
    An unique rsrc identifier string will then be created.

    @param  rsrcName    a resource name (usually a file path), or an empty string
    @return             a pointer to buf, which contains the result
*/
nString
nResourceServer::GetResourceId(const nString& rsrcName)
{
    nString str;
    if (rsrcName.IsEmpty()) str.Format("unique%d", this->uniqueId++);
    else
    {
        int len = rsrcName.Length();
        int numChars = N_MAXNAMELEN;
        int offset = len - N_MAXNAMELEN;
        if (offset < 0)
        {
            numChars += offset;
            offset = 0;
        }
        str = rsrcName.SubString(offset, numChars);
        str.ReplaceChars("\\/:*?\"<>|.", '_');
    }
    return str;
}

//------------------------------------------------------------------------------
/**
    Find the right resource root object for a given resource type.

    @param  rsrcType    the resource type
    @return             the root object
*/
nRoot*
nResourceServer::GetResourcePool(nResource::Type rsrcType)
{
    switch (rsrcType)
    {
        case nResource::Mesh:              return this->meshPool.get();
        case nResource::Texture:           return this->texPool.get();
        case nResource::Shader:            return this->shdPool.get();
        case nResource::Animation:         return this->animPool.get();
        case nResource::SoundResource:     return this->sndResPool.get();
        case nResource::Other:             return this->otherPool.get();
        default:
            // can't happen
            n_assert(false);
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    Find a resource object by resource type and name.

    @param  rsrcName    the rsrc name
    @param  rsrcType    resource type
    @return             pointer to resource object, or 0 if not found
*/
nResource*
nResourceServer::FindResource(const nString& rsrcName, nResource::Type rsrcType)
{
    n_assert(nResource::InvalidResourceType != rsrcType);
    nString rsrcId = this->GetResourceId(rsrcName);
    nRoot* rsrcPool = this->GetResourcePool(rsrcType);
    n_assert(rsrcPool);
    return (nResource*) rsrcPool->Find(rsrcId.Get());
}

//------------------------------------------------------------------------------
/**
    Create a new possible shared resource object. Bumps refcount on an
    existing resource object. Pass a zero rsrcName if a (non-shared) resource
    should be created.

    @param  className   the Nebula class name
    @param  rsrcName    the rsrc name (for resource sharing), can be 0
    @param  rsrcType    resource type
    @return             pointer to resource object
*/
nResource*
nResourceServer::NewResource(const nString& className, const nString& rsrcName, nResource::Type rsrcType)
{
    n_assert(nResource::InvalidResourceType != rsrcType);

    nString rsrcId = this->GetResourceId(rsrcName);
    nRoot* rsrcPool = this->GetResourcePool(rsrcType);
    n_assert(rsrcPool);

    // see if resource exist
    nResource* obj = (nResource*) rsrcPool->Find(rsrcId.Get());
    if (obj)
    {
        // exists, bump refcount and return
        obj->AddRef();
    }
    else
    {
        // create new resource object
        nKernelServer::Instance()->PushCwd(rsrcPool);
        obj = (nResource*) nKernelServer::Instance()->New(className.Get(), rsrcId.Get());
        nKernelServer::Instance()->PopCwd();
        n_assert(obj);
    }
    return obj;
}

//------------------------------------------------------------------------------
/**
    Unload all resources matching the given resource type mask.

    @param  rsrcTypeMask    a mask of nResource::Type values
*/
void
nResourceServer::UnloadResources(int rsrcTypeMask)
{
    int i;
    for (i = 1; i < nResource::InvalidResourceType; i <<= 1)
    {
        if (0 != (rsrcTypeMask & i))
        {
            nRoot* rsrcPool = this->GetResourcePool((nResource::Type) i);
            n_assert(rsrcPool);
            nResource* rsrc;
            for (rsrc = (nResource*) rsrcPool->GetHead(); rsrc; rsrc = (nResource*) rsrc->GetSucc())
            {
                rsrc->Unload();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Load all resources matching the given resource type mask. Returns false
    if any of the resources didn't load correctly.

    IMPLEMENTATION NOTE: since the Bundle resource type is defined
    before all other resource types, it is guaranteed that bundled
    resources are loaded before all others.

    @param  rsrcTypeMask  a resource type
    @return               true if all resources loaded correctly
*/
bool
nResourceServer::LoadResources(int rsrcTypeMask)
{
    int i;
    bool retval = true;
    for (i = 1; i < nResource::InvalidResourceType; i <<= 1)
    {
        if (0 != (rsrcTypeMask & i))
        {
            nRoot* rsrcPool = this->GetResourcePool((nResource::Type) i);
            n_assert(rsrcPool);

            nResource* rsrc;
            for (rsrc = (nResource*) rsrcPool->GetHead(); rsrc; rsrc = (nResource*) rsrc->GetSucc())
            {
                // NOTE: if the resource is bundled, it could've been loaded already
                // (if this is the actual resource object which has been created by the
                // bundle, thus we check if the resource has already been loaded)
                if (!rsrc->IsLoaded())
                {
                    retval &= rsrc->Load();
                }
            }
        }
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
    Calls nResource::OnLost() on all resources defined in the resource
    type mask.

    @param  rsrcTypeMask    a mask of nResource::Type values
*/
void
nResourceServer::OnLost(int rsrcTypeMask)
{
    int i;
    for (i = 1; i < nResource::InvalidResourceType; i <<= 1)
    {
        if (0 != (rsrcTypeMask & i))
        {
            nRoot* rsrcPool = this->GetResourcePool((nResource::Type) i);
            n_assert(rsrcPool);
            nResource* rsrc;
            for (rsrc = (nResource*) rsrcPool->GetHead(); rsrc; rsrc = (nResource*) rsrc->GetSucc())
            {
                rsrc->OnLost();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Calls nResource::OnRestored() on all resources defined in the resource
    type mask.

    @param  rsrcTypeMask    a resource type
*/
void
nResourceServer::OnRestored(int rsrcTypeMask)
{
    int i;
    for (i = 1; i < nResource::InvalidResourceType; i <<= 1)
    {
        if (0 != (rsrcTypeMask & i))
        {
            nRoot* rsrcPool = this->GetResourcePool((nResource::Type) i);
            n_assert(rsrcPool);

            nResource* rsrc;
            for (rsrc = (nResource*) rsrcPool->GetHead(); rsrc; rsrc = (nResource*) rsrc->GetSucc())
            {
                rsrc->OnRestored();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Wakeup the loader thread. This will simply signal the jobList.
*/
void
nResourceServer::ThreadWakeupFunc(nThread* thread)
{
    nResourceServer* self = (nResourceServer*) thread->LockUserData();
    thread->UnlockUserData();
    self->jobList.SignalEvent();
}

//------------------------------------------------------------------------------
/**
    The background loader thread func. This will sit on the jobList until
    it is signaled (when new jobs arrive), and for each job in the job
    list, it will invoke the LoadResource() method of the resource object
    and remove the resource object from the job list.
*/
int
N_THREADPROC
nResourceServer::LoaderThreadFunc(nThread* thread)
{
    // tell thread object that we have started
    thread->ThreadStarted();

    // get pointer to thread server object
    nResourceServer* self = (nResourceServer*) thread->LockUserData();
    thread->UnlockUserData();

    // sit on the jobList signal until new jobs arrive
    do
    {
        // do nothing until job list becomes signalled
        self->jobList.WaitEvent();

        // does our boss want us to shut down?
        if (!thread->ThreadStopRequested())
        {
            // get all pending jobs
            while (self->jobList.GetHead())
            {
                // keep the job object from joblist
                self->jobList.Lock();
                nDataNode* jobNode = (nDataNode*)self->jobList.RemHead();
                nResource* res = (nResource*) jobNode->GetPtr();

                // take the resource's mutex and lock the resource,
                // this prevents the resource to be deleted
                res->pMutex->Lock();
                self->jobList.Unlock();

                res->LoadResource();
				res->pMutex->Unlock();

                // proceed to next job
            }
        }
    }
    while (!thread->ThreadStopRequested());

    // tell thread object that we are done
    thread->ThreadHarakiri();
    return 0;
}

//------------------------------------------------------------------------------
/**
    Start the loader thread.
*/
void
nResourceServer::StartLoaderThread()
{
    n_assert(0 == this->loaderThread);

    // give the thread sufficient stack size (2.5 MB) and a below
    // normal priority (the purpose of the thread is to guarantee
    // a smooth framerate despite dynamic resource loading after all)
    this->loaderThread = n_new(nThread(LoaderThreadFunc, nThread::Normal, 2500000, ThreadWakeupFunc, 0, this));
}

//------------------------------------------------------------------------------
/**
    Shutdown the loader thread.
*/
void
nResourceServer::ShutdownLoaderThread()
{
    n_assert(this->loaderThread);
    n_delete(this->loaderThread);
    this->loaderThread = 0;

    // clear the job list
    this->jobList.Lock();
    while (this->jobList.RemHead());
    this->jobList.Unlock();
}

//------------------------------------------------------------------------------
/**
    Add a resource to the job list for asynchronous loading.
*/
void
nResourceServer::AddLoaderJob(nResource* res)
{
    n_assert(res);
    n_assert(!res->IsPending());
    n_assert(!res->IsLoaded());
    this->jobList.Lock();
    this->jobList.AddTail(&(res->jobNode));
    this->jobList.Unlock();
    this->jobList.SignalEvent();
}

//------------------------------------------------------------------------------
/**
    Remove a resource from the job list for asynchronous loading.
*/
void
nResourceServer::RemLoaderJob(nResource* res)
{
    n_assert(res);
    this->jobList.Lock();
    if (res->IsPending())
    {
        res->jobNode.Remove();
    }
    this->jobList.Unlock();
}
