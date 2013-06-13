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

	PResource New = (CResource*)Type.CreateInstance(&UID);
	UIDToResource.Add(UID, New.GetUnsafe());
	return New;
}
//---------------------------------------------------------------------

int IResourceManager::DeleteResource(CStrID UID)
{
	//!!!duplicate search!
	PResource* ppRsrc = UIDToResource.Get(UID);
	if (!ppRsrc) return -1;
	int RC = (*ppRsrc).IsValid() ? (*ppRsrc)->GetRefCount() : 0;
	UIDToResource.Erase(UID);
	return RC - 1;
}
//---------------------------------------------------------------------

// Returns released memory size
DWORD IResourceManager::UnloadUnreferenced()
{
	DWORD Released = 0;
	CHashTable<CStrID, PResource>::CIterator It = UIDToResource.Begin();
	while (!It.IsEnd())
		if (It.GetValue()->GetRefCount() == 1)
		{
			Released += It.GetValue()->GetSizeInBytes();
			It.GetValue()->Unload();
		}
	return Released;
}
//---------------------------------------------------------------------

// Returns released memory size
DWORD IResourceManager::DeleteUnreferenced()
{
	//!!!SLOW and UGLY!
	//!!!allow deletion on iteration or use array/pool for the main storage and weak ptr map only for indexing?
	//!!!write deletion by CIterator!
	//???migrate to bullet or some other hash map?

	nArray<CStrID> ToDelete;

	DWORD Released = 0;
	CHashTable<CStrID, PResource>::CIterator It = UIDToResource.Begin();
	while (!It.IsEnd())
		if (It.GetValue()->GetRefCount() == 1)
		{
			Released += It.GetValue()->GetSizeInBytes();
			It.GetValue()->Unload();
			ToDelete.Append(It.GetKey());
		}

	for (int i = 0; i < ToDelete.GetCount(); ++i)
		UIDToResource.Erase(ToDelete[i]);

	return Released;
}
//---------------------------------------------------------------------

// Returns released memory size
DWORD IResourceManager::FreeMemory(DWORD DesiredBytes)
{
	//!!!unload the least referenced or least recently used first!
	return 0;
}
//---------------------------------------------------------------------

DWORD IResourceManager::GetMemoryUsed() //const //!!!need const CIterator!
{
	DWORD Total = 0;
	CHashTable<CStrID, PResource>::CIterator It = UIDToResource.Begin();
	while (!It.IsEnd())
		Total += It.GetValue()->GetSizeInBytes();
	return Total;
}
//---------------------------------------------------------------------

}
