#include "ResourceManager.h"

#include <Resources/Resource.h>

namespace Resources
{

PResource IResourceManager::CreateResource(CStrID UID, const Core::CRTTI& Type)
{
	n_assert2_dbg(Type.IsDerivedFrom(CResource::RTTI), (Type.GetName() + " is not a CResource or a subclass!").CStr());

	if (!UID.IsValid())
	{
		char ID[20];
		sprintf(ID, "Rsrc%d", UIDCounter++);
		UID = CStrID(ID);
		n_assert_dbg(!UIDToResource.Contains(UID));
	}

	if (UIDToResource.Contains(UID)) return NULL;

	PResource New = (CResource*)Type.Create(&UID); //n_new(TRsrc)(UID);
	UIDToResource.Add(UID, New.GetUnsafe());
	return New;
}
//---------------------------------------------------------------------

int IResourceManager::UnloadResource(CStrID UID)
{
	//!!!duplicate search!
	PResource* ppRsrc = UIDToResource.Get(UID);
	if (!ppRsrc) return -1;
	int RC = (*ppRsrc).IsValid() ? (*ppRsrc)->GetRefCount() : 0;
	UIDToResource.Erase(UID);
	return RC - 1;
}
//---------------------------------------------------------------------

}
