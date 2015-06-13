#pragma once
#ifndef __DEM_L1_RESOURCE_MANAGER_H__
#define __DEM_L1_RESOURCE_MANAGER_H__

#include <Data/StringID.h>
#include <Data/SimpleString.h>
#include <Data/HashTable.h>
#include <Data/Singleton.h>

// Resource manager controls resource loading, lifetime, uniquity, and serves as
// a hub for accessing any types of assets in an abstract way.
// Resources are identified and located by URI (Uniform Resource Identifier).
// Resource ID is its URI expanded and made relative to the RootPath, if possible.
// It is necessary for file resources for two reasons:
// 1. We avoid wasting memory for an absolute path to data root in each resource ID.
// 2. We make resource IDs independent from data root location, so resource IDs
//    will always stay the same no matter from which place we run our application.
// You always can leave RootPath empty and use absolute resource IDs.

namespace Resources
{
typedef Ptr<class CResource> PResource;

#define ResourceMgr Resources::ÑResourceManager::Instance()

class ÑResourceManager //???refcounted / object?
{
	__DeclareSingleton(ÑResourceManager);

protected:

	//???what about empty hand-created resources, that aren't from file?
	//PResource	RegisterResource(CStrID ID, PrecreatedRsrcObject); //???

	//!!!???some pool?! if pool, hash table can store weak ptrs

	Data::CSimpleString				RootPath;
	CHashTable<CStrID, PResource>	Registry;

public:

	ÑResourceManager(DWORD HashTableCapacity = 256): Registry(HashTableCapacity) { __ConstructSingleton; }
	~ÑResourceManager() { __DestructSingleton; }

	PResource	RegisterResource(const char* pURI); //!!!use URI structue, pre-parsed! one string w/ptrs to parts
};

}

#endif
