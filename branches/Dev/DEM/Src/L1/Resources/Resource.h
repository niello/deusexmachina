#pragma once
#ifndef __DEM_L1_RESOURCE_H__
#define __DEM_L1_RESOURCE_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Resource wraps a piece of abstract data, used by other systems. It can be loaded and managed by
// resource system, so clients needn't to care about loading, unloading and LOD of resources.

namespace Resources
{
typedef Ptr<class CResourceLoader> PResourceLoader;

enum EResourceState
{
	Rsrc_NotLoaded,
	Rsrc_Loaded,
	Rsrc_Failed
	//Rsrc_Pending - for async
};

class CResource: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	friend class CResourceServer; // To set UID

	CStrID			UID;
	EResourceState	State;
	nString			FileName;
	PResourceLoader	Loader; //???store filename inside a loader, if it is needed?

	CResource(): State(Rsrc_NotLoaded) {} // Create only by RsrcSrv

public:

	virtual ~CResource();

	bool			Load(LPCSTR pFileName, CResourceLoader* pLoader);
	bool			Reload();
	void			Unload();

	CStrID			GetUID() const { return UID; }
	EResourceState	GetState() const { return State; }
	bool			IsLoaded() const { return State == Rsrc_Loaded; }
};

typedef Ptr<CResource> PResource;

}

#endif
