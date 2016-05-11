#include "ResourceManager.h"

#include <Resources/Resource.h>
#include <Resources/ResourceLoader.h>

namespace Resources
{
__ImplementSingleton(CResourceManager);

PResource CResourceManager::RegisterResource(CStrID URI) //???need? avoid recreating StrID of already registered resource, but mb create only here?
{
	//!!!TODO!
	//test if CStrID is a valid expanded URI! or no need?
	PResource Rsrc;
	if (!Registry.Get(URI, Rsrc))
	{
		Rsrc = n_new(CResource); //???pool? UID in constructor?
		Rsrc->SetUID(URI);
		Registry.Add(URI, Rsrc);
	}
	return Rsrc;
}
//---------------------------------------------------------------------

PResource CResourceManager::RegisterResource(const char* pURI)
{
	//!!!TODO!
	// Absolutize (expand) URI:
	// - resolve assigns
	// - if file system path, make relative to resource root (save space)
	CStrID UID = CStrID(pURI);
	PResource Rsrc;
	if (!Registry.Get(UID, Rsrc))
	{
		Rsrc = n_new(CResource); //???pool? UID in constructor?
		Rsrc->SetUID(UID);
		Registry.Add(UID, Rsrc);
	}
	return Rsrc;
}
//---------------------------------------------------------------------

bool CResourceManager::RegisterDefaultLoader(const char* pFmtExtension, const Core::CRTTI* pRsrcType, CResourceLoader* pLoader, bool CloneOnCreate)
{
	if (pRsrcType && pLoader && !pLoader->GetResultType().IsDerivedFrom(*pRsrcType)) FAIL;

	CLoaderKey Key;
	Key.pRsrcType = pRsrcType;

	UPTR ExtLen = strlen(pFmtExtension) + 1;
	char* pFmtExtensionLower = (char*)_malloca(ExtLen);
	memcpy(pFmtExtensionLower, pFmtExtension, ExtLen);
	_strlwr_s(pFmtExtensionLower, ExtLen);
	Key.Extension = CStrID(pFmtExtensionLower);
	_freea(pFmtExtensionLower);

	if (pLoader)
	{
		CLoaderRec Rec;
		Rec.Loader = pLoader;
		Rec.CloneOnCreate = CloneOnCreate;
		DefaultLoaders.Add(Key, Rec, true);
	}
	else DefaultLoaders.Remove(Key);

	OK;
}
//---------------------------------------------------------------------

PResourceLoader CResourceManager::CreateDefaultLoader(const char* pFmtExtension, const Core::CRTTI* pRsrcType)
{
	CLoaderKey Key;

	UPTR ExtLen = strlen(pFmtExtension) + 1;
	char* pFmtExtensionLower = (char*)_malloca(ExtLen);
	memcpy(pFmtExtensionLower, pFmtExtension, ExtLen);
	_strlwr_s(pFmtExtensionLower, ExtLen);
	Key.Extension = CStrID(pFmtExtensionLower);
	_freea(pFmtExtensionLower);

#ifdef _DEBUG
	const Core::CRTTI* pStartingRsrcType = pRsrcType;
#endif

	// Try to find loader for any resource in the class hierarchy
	CLoaderRec* pRec = NULL;
	while (!pRec)
	{
		Key.pRsrcType = pRsrcType;
		pRec = DefaultLoaders.Get(Key);
		if (!pRsrcType) break;
		pRsrcType = pRsrcType->GetParent();
	}

	if (!pRec)
	{
#ifdef _DEBUG
		Sys::Error("No default loader associated for '%s' files and '%s' resource type\n", Key.Extension.CStr(), pStartingRsrcType ? pStartingRsrcType->GetName().CStr() : "<any>");
#endif
		return NULL;
	}

	return pRec->CloneOnCreate ? pRec->Loader->Clone() : pRec->Loader;
}
//---------------------------------------------------------------------

void CResourceManager::LoadResourceSync(CResource& Rsrc, CResourceLoader& Loader)
{
	//???process URI and IO here as common code? setup IO::CStream

	Rsrc.SetState(Rsrc_LoadingInProgress);
	if (Loader.Load(Rsrc)) Rsrc.SetState(Rsrc_Loaded);
	else Rsrc.SetState(Rsrc_LoadingFailed);
}
//---------------------------------------------------------------------

void CResourceManager::LoadResourceAsync(CResource& Rsrc, CResourceLoader& Loader)
{
	Sys::Error("IMPLEMENT ME!!!\n");
	//create job(task) that will call this method with Async = false from another thread
	//must return some handle to cancel/wait/check task
	//???store async handle in a resource?
	Rsrc.SetState(Rsrc_LoadingRequested);
}
//---------------------------------------------------------------------

}