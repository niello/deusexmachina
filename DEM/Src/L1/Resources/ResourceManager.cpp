#include "ResourceManager.h"

#include <Resources/Resource.h>

namespace Resources
{

void IResourceManager::ReleaseResource(CResource* pResource)
{
	n_error("This was designed for weak ptrs in IDToResource!");
	//n_assert_dbg(pResource && !pResource->GetRefCount());
	//IDToResource.Erase(pResource->GetUID());
}
//---------------------------------------------------------------------

PResource IResourceManager::GetResource(CStrID UID)
{
	PResource* ppRsrc = IDToResource.Get(UID);

	if (ppRsrc) return *ppRsrc;

	if (!UID.IsValid())
	{
		char ID[20];
		sprintf(ID, "Rsrc%d", UIDCounter++);
		UID = CStrID(ID);
		n_assert_dbg(!IDToResource.Contains(UID));
	}

	if (IDToResource.Contains(UID)) return NULL;

	PResource New = CreateResource(UID); //????can avoid virtuality?
	IDToResource.Add(UID, New);
	return New;
}
//---------------------------------------------------------------------

}
