#include "ResourceManager.h"

#include <Resources/Resource.h>

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
	n_assert(false);
}
//---------------------------------------------------------------------

}