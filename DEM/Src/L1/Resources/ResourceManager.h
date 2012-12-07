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

template<class TRsrc>
class CResourceManager
{
public:

	typedef Ptr<TRsrc> PTRsrc;

protected:

	static DWORD UIDCounter;

	//!!!???some pool?! if pool, hash table can store weak ptrs
	CHashTable<CStrID, PTRsrc>	IDToResource;

public:

	PTRsrc	GetResource(CStrID UID);
	//bool	LoadResource(CStrID UID, bool ForceReload = true); //???here or in resource only?
};

template<class TRsrc>
DWORD CResourceManager<TRsrc>::UIDCounter = 0;

template<class TRsrc>
Ptr<TRsrc> CResourceManager<TRsrc>::GetResource(CStrID UID)
{
	PTRsrc* ppRsrc = IDToResource.Get(UID);

	if (ppRsrc) return *ppRsrc;

	//n_assert(RsrcClass.IsDerivedFrom(CResource::RTTI));

	if (!UID.IsValid())
	{
		char ID[20];
		sprintf(ID, "Rsrc%d", UIDCounter++);
		UID = CStrID(ID);
		n_assert_dbg(!IDToResource.Contains(UID));
	}

	if (IDToResource.Contains(UID)) return NULL;

	Ptr<TRsrc> New = n_new(TRsrc)(UID);
	IDToResource.Add(UID, New);
	return New;
}
//---------------------------------------------------------------------

}

#endif
