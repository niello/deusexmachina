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
	// Absolutize URI
	// Get CStrID
	// If registered return it
	// else create empty container
	PResource Rsrc = n_new(CResource);
	Rsrc->SetUID(CStrID(pURI));
	return Rsrc;
}
//---------------------------------------------------------------------

void CResourceManager::RegisterDefaultLoader(const char* pFmtExtension, const Core::CRTTI* pRsrcType, const Core::CRTTI* pLoaderType)
{
	CLoaderKey Key;
	Key.Extension = CStrID(pFmtExtension);
	Key.pRsrcType = pRsrcType;
	DefaultLoaders.Add(Key, pLoaderType, true);
}
//---------------------------------------------------------------------

//!!!check IsDerivedFrom! select the best matching loader (minimal hierarchy offset from requested type)!
PResourceLoader CResourceManager::CreateDefaultLoader(const char* pFmtExtension, const Core::CRTTI* pRsrcType)
{
	CLoaderKey Key;
	Key.Extension = CStrID(pFmtExtension);
	Key.pRsrcType = pRsrcType;
	const Core::CRTTI** ppRTTI = DefaultLoaders.Get(Key);
	return ppRTTI ? (CResourceLoader*)(*ppRTTI)->CreateClassInstance() : NULL;
}
//---------------------------------------------------------------------

void CResourceManager::LoadResource(CResource& Rsrc, CResourceLoader& Loader, bool Async)
{
	if (Async)
	{
		Sys::Error("IMPLEMENT ME!!!\n");
		//create job(task) that will call this method with Async = false from another thread
		//must return some handle to cancel/wait/check task
		//???store async handle in a resource?
		Rsrc.SetState(Rsrc_LoadingRequested);
	}
	else
	{
		Rsrc.SetState(Rsrc_LoadingInProgress);
		if (Loader.Load(Rsrc)) Rsrc.SetState(Rsrc_Loaded);
		else Rsrc.SetState(Rsrc_LoadingFailed);
	}
}
//---------------------------------------------------------------------

}