#pragma once
#ifndef __DEM_L1_RESOURCE_MANAGER_H__
#define __DEM_L1_RESOURCE_MANAGER_H__

#include <Core/Ptr.h>
#include <Data/StringID.h>
#include <util/HashTable.h>

// Template resource manager that allows to manage different resources separately.
// Each manager has its ID namespace, so UIDs are unique only in a scope of one manager.
// All managers can use the same singleton loading task dispatcher, that in turn can
// feed job manager with async resource loading requests, keeping track of some statistics
// for both profiling and level loading progress tracking.

//!!!???derive CDescManager - for resources that are loaded from HRD?! base tpl support like in items?

namespace Resources
{
typedef Ptr<class CResource> PResource;

class IResourceManager
{
protected:

	DWORD UIDCounter;

	//!!!???some pool?! if pool, hash table can store weak ptrs

	CHashTable<CStrID, PResource>	IDToResource;

	//PResource Placeholder;

public:

	IResourceManager(): UIDCounter(0) {}

	PResource GetResource(CStrID UID);
};

template<class TRsrc>
class CResourceManager: public IResourceManager
{
public:

	bool				AddResource(Ptr<TRsrc> NewRsrc);
	Ptr<TRsrc>			GetTypedResource(CStrID UID) { return (TRsrc*)GetResource(UID).get_unsafe(); }
	Ptr<TRsrc>			GetOrCreateTypedResource(CStrID UID);
};

template<class TRsrc>
bool CResourceManager<TRsrc>::AddResource(Ptr<TRsrc> NewRsrc)
{
	if (!NewRsrc.isvalid() || !NewRsrc->GetUID().IsValid()) FAIL;
	IDToResource.Add(NewRsrc->GetUID(), (CResource*)NewRsrc.get_unsafe());
	OK;
}
//---------------------------------------------------------------------

template<class TRsrc>
Ptr<TRsrc> CResourceManager<TRsrc>::GetOrCreateTypedResource(CStrID UID)
{
	PResource* ppRsrc = IDToResource.Get(UID);
	if (ppRsrc) return (TRsrc*)(*ppRsrc).get_unsafe();

	if (!UID.IsValid())
	{
		char ID[20];
		sprintf(ID, "Rsrc%d", UIDCounter++);
		UID = CStrID(ID);
		n_assert_dbg(!IDToResource.Contains(UID));
	}

	if (IDToResource.Contains(UID)) return NULL;

	Ptr<TRsrc> New = n_new(TRsrc)(UID);
	IDToResource.Add(UID, New.get_unsafe());
	return New;
}
//---------------------------------------------------------------------

}

#endif
