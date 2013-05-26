#pragma once
#ifndef __DEM_L1_RESOURCE_MANAGER_H__
#define __DEM_L1_RESOURCE_MANAGER_H__

#include <Core/Ptr.h>
#include <Core/RTTI.h>
#include <Data/StringID.h>
#include <util/HashTable.h>

// Template resource manager that allows to manage different resources separately.
// Resource manager ensures that each shared resource is loaded in memory exactly once.
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

	CHashTable<CStrID, PResource>	UIDToResource;

	//PResource Placeholder;

public:

	IResourceManager(): UIDCounter(0) {}

	PResource	CreateResource(CStrID UID, const Core::CRTTI& Type);
	PResource	GetResource(CStrID UID);
	int			DeleteResource(CStrID UID); // returns remaining refcount, if 0, was really unloaded, if -1, was not found
	bool		ResourceExists(CStrID UID) const { return UIDToResource.Contains(UID); }

	DWORD		UnloadUnreferenced();
	DWORD		DeleteUnreferenced();
	DWORD		FreeMemory(DWORD DesiredBytes);
	DWORD		GetMemoryUsed();
};

inline PResource IResourceManager::GetResource(CStrID UID)
{
	PResource* ppRsrc = UIDToResource.Get(UID);
	return ppRsrc ? *ppRsrc : NULL;
}
//---------------------------------------------------------------------

template<class TRsrc>
class CResourceManager: public IResourceManager
{
public:

	Ptr<TRsrc>		CreateTypedResource(CStrID UID, const Core::CRTTI& Type = TRsrc::RTTI);
	template<class TSubRsrc>
	Ptr<TSubRsrc>	CreateTypedResource(CStrID UID) { return (TSubRsrc*)CreateTypedResource(UID, TSubRsrc::RTTI).GetUnsafe(); }
	Ptr<TRsrc>		GetTypedResource(CStrID UID) { return (TRsrc*)GetResource(UID).GetUnsafe(); }
	Ptr<TRsrc>		GetOrCreateTypedResource(CStrID UID, const Core::CRTTI& Type = TRsrc::RTTI);
	template<class TSubRsrc>
	Ptr<TSubRsrc>	GetOrCreateTypedResource(CStrID UID) { return (TSubRsrc*)GetOrCreateTypedResource(UID, TSubRsrc::RTTI).GetUnsafe(); }
};

template<class TRsrc>
Ptr<TRsrc> CResourceManager<TRsrc>::CreateTypedResource(CStrID UID, const Core::CRTTI& Type)
{
	n_assert2_dbg(Type.IsDerivedFrom(TRsrc::RTTI), (Type.GetName() + " is not a " + TRsrc::RTTI.GetName() + " or a subclass!").CStr());
	return (TRsrc*)CreateResource(UID, Type).GetUnsafe();
}
//---------------------------------------------------------------------

template<class TRsrc>
Ptr<TRsrc> CResourceManager<TRsrc>::GetOrCreateTypedResource(CStrID UID, const Core::CRTTI& Type)
{
	PResource* ppRsrc = UIDToResource.Get(UID);
	if (ppRsrc) return (TRsrc*)(*ppRsrc).GetUnsafe();
	return CreateTypedResource(UID, Type);
}
//---------------------------------------------------------------------

}

#endif
