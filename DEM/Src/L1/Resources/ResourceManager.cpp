#include "ResourceManager.h"

#include <Resources/Resource.h>

namespace Resources
{

PResource IResourceManager::GetResource(CStrID UID)
{
	PResource* ppRsrc = IDToResource.Get(UID);
	return ppRsrc ? *ppRsrc : NULL;
}
//---------------------------------------------------------------------

}
