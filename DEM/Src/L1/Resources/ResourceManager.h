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

	friend class CResource; // For ReleaseResource

	DWORD UIDCounter;

	//!!!???some pool?! if pool, hash table can store weak ptrs

	CHashTable<CStrID, PResource>	IDToResource;

	//PResource Placeholder;

	virtual PResource	CreateResource(CStrID UID) = 0;
	void				ReleaseResource(CResource* pResource);

public:

	IResourceManager(): UIDCounter (0) {}

	PResource	GetResource(CStrID UID);
};

template<class TRsrc>
class CResourceManager: public IResourceManager
{
protected:

	virtual PResource	CreateResource(CStrID UID) { return n_new(TRsrc)(UID, this); }

public:

	Ptr<TRsrc>			GetTypedResource(CStrID UID) { return (TRsrc*)GetResource(UID).get_unsafe(); }
};

}

#endif
