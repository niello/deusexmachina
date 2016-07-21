#pragma once
#ifndef __DEM_L1_RESOURCE_H__
#define __DEM_L1_RESOURCE_H__

#include <Resources/ResourceObject.h>
#include <Data/StringID.h>

// A container that wraps some actual resource object along with any information
// required for its management. This class is not intended to be a base class for
// specific resources. Inherit from a CResourceObject instead.

namespace Resources
{
typedef Ptr<class CResourceLoader> PResourceLoader;
typedef Ptr<class CResourceGenerator> PResourceGenerator;

enum EResourceState
{
	Rsrc_NotLoaded,
	Rsrc_LoadingRequested,
	Rsrc_LoadingInProgress,
	Rsrc_Loaded,
	Rsrc_LoadingFailed,
	Rsrc_LoadingCancelled
};

//!!!different resource mgmt flags! like autoreload on invalidity detected etc

class CResource: public Data::CRefCounted
{
protected:

	CStrID					UID;			// If resource is not created manually, UID stores URI that locates resource data
	mutable EResourceState	State;
	UPTR					ByteSize;
	PResourceObject			Object;			// Actual object, such as a texture or a game object description
	//???PResourceObject		Placeholder; //???here or register per RTTI in Mgr?
	PResourceLoader			Loader;			// Optional, for recreation of lost resource object
	PResourceGenerator		Generator;		// Optional, for recreation of lost resource object

public:

	CResource(): ByteSize(0), State(Rsrc_NotLoaded) {}

	CResourceObject*	GetObject() const;
	template<class T>
	T*					GetObject() const;

	CStrID				GetUID() const { return UID; }
	UPTR				GetSizeInBytes() const { return ByteSize; }
	EResourceState		GetState() const { return State; } //!!!must be thread-safe!
	bool				IsLoaded() const { return State == Rsrc_Loaded; } //!!!must be thread-safe!
	CResourceLoader*	GetLoader() const { return Loader.GetUnsafe(); }

	// For internal use by CResourceManager
	void				SetUID(CStrID NewUID) { UID = NewUID; }
	void				SetState(EResourceState NewState) const { State = NewState; } //!!!must be thread-safe!
	void				Init(PResourceObject NewObject, PResourceLoader NewLoader = NULL, PResourceGenerator NewGenerator = NULL) { Object = NewObject; Loader = NewLoader; Generator = NewGenerator; } //!!!must be thread-safe!
};

typedef Ptr<CResource> PResource;

template<class T> inline T* CResource::GetObject() const
{
	CResourceObject* pObj = GetObject();
	return (pObj && pObj->IsA<T>()) ? (T*)pObj : NULL;
}
//--------------------------------------------------------------------

}

#endif
