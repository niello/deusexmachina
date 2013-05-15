#include "ResourceManager.h"

#include <Resources/Resource.h>

namespace Resources
{

PResource IResourceManager::GetResource(CStrID UID)
{
	PResource* ppRsrc = UIDToResource.Get(UID);
	return ppRsrc ? *ppRsrc : NULL;
}
//---------------------------------------------------------------------

}
