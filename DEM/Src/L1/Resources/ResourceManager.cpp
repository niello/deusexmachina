#include "ResourceManager.h"

#include <Resources/Resource.h>
#include <Resources/ResourceLoader.h>

namespace Resources
{
__ImplementSingleton(CResourceManager);

PResource CResourceManager::RegisterResource(CStrID URI) //???need? avoid recreating StrID, but mb create only here?
{
	// If registered return it
	// else create empty container
	return NULL;
}
//---------------------------------------------------------------------

PResource CResourceManager::RegisterResource(const char* pURI)
{
	// If registered return it
	// else create empty container
	return NULL;
}
//---------------------------------------------------------------------

PResourceLoader CResourceManager::CreateDefaultLoader(const char* pFmtExtension, const Core::CRTTI* pRsrcType)
{
	return NULL;
}
//---------------------------------------------------------------------

void CResourceManager::LoadResource(PResource Rsrc, PResourceLoader Loader, bool Async)
{
	if (Rsrc.IsNullPtr() || Loader.IsNullPtr()) return;

	CResource* pRsrc = Rsrc.GetUnsafe();

	if (Async)
	{
		Sys::Error("IMPLEMENT ME!!!\n");
		//create job(task) that will call this method with Async = false from another thread
		//must return some handle to cancel/wait/check task
		//???store async handle in a resource?
		pRsrc->SetState(Rsrc_LoadingRequested);
	}
	else
	{
		pRsrc->SetState(Rsrc_LoadingInProgress);
		if (Loader->Load(*pRsrc)) pRsrc->SetState(Rsrc_Loaded);
		else pRsrc->SetState(Rsrc_LoadingFailed);
	}
}
//---------------------------------------------------------------------

}