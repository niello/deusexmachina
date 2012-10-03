//------------------------------------------------------------------------------
//  nresource_main.cc
//  (C) 2001 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "resource/nresource.h"
#include "resource/nresourceserver.h"
#include "kernel/nkernelserver.h"

nNebulaClass(nResource, "nroot");

uint nResource::uniqueIdCounter = 0;

nResource::~nResource()
{
    n_assert(this->GetState() == Unloaded);

    // if we are pending for an async load, we must remove ourselves from the loader job list.
	if (nResourceServer::HasInstance())
        nResourceServer::Instance()->RemLoaderJob(this);
}

//------------------------------------------------------------------------------
/**
    Request to load a resource in synchronous or asynchronous mode. Will
    care about multithreading issues and invoke the LoadResource() method
    which should be overriden by subclasses to implement the actual loading.
    Subclasses must indicate with the CanLoadAsync() method whether they
    support asynchronous loading or not.

    NOTE: in asynchronous mode, the method will return true although the
    resource data is not available yet. Use the IsValid() method to
    check when the resource data is available.

    @return     true if all ok,
*/
bool
nResource::Load()
{
    // if we are already loaded, do nothing (we still may be
    // in in-operational state, either Lost, or Restored)
    if (Unloaded != this->GetState()) return true;

    #ifndef __NEBULA_NO_THREADS__
    if (this->GetAsyncEnabled() && this->CanLoadAsync())
    {
        if (this->IsPending()) return true;
        nResourceServer::Instance()->AddLoaderJob(this);
        return true;
    }
    #endif
    // the synchronous case is simply
    return this->LoadResource();
}

//------------------------------------------------------------------------------
/**
    Unload the resource data, freeing runtime resources. This method will call
    the protected virtual UnloadResources() method which should be overriden
    by subclasses.
    This method works in sync and async mode and care about the multi-threading
    issues before and after calling LoadResources();
*/
void nResource::Unload()
{
    // remove from loader list, if pending for async load
    if (nResourceServer::HasInstance())
        nResourceServer::Instance()->RemLoaderJob(this);
    if (Unloaded != this->GetState())
        this->UnloadResource();
}
