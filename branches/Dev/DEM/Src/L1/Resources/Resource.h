#pragma once
#ifndef __DEM_L1_RESOURCE_H__
#define __DEM_L1_RESOURCE_H__

#include <Data/RefCounted.h>
#include <Data/StringID.h>

// A container that wraps some actual resource object along with any information
// required for its management. This class is not intended to be a base class for
// specific resources. Inherit from a CResourceObject instead.

namespace Resources
{
typedef Ptr<class CResourceObject> PResourceObject;
typedef Ptr<class CResourceLoader> PResourceLoader;

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

	CStrID				UID;		// If resource is not created manually, UID stores URI that locates resource data
	EResourceState		State;
	DWORD				ByteSize;
	PResourceObject		Object;		// Actual object, such as a texture or a game object description
	PResourceLoader		Loader;		// Optional, for recreation of lost resource object

public:

	CResource(): ByteSize(0), State(Rsrc_NotLoaded) {}

	CStrID			GetUID() const { return UID; }
	DWORD			GetSizeInBytes() const { return ByteSize; }
	EResourceState	GetState() const { return State; }
	bool			IsLoaded() const { return State == Rsrc_Loaded; }
};

typedef Ptr<CResource> PResource;

}

#endif
