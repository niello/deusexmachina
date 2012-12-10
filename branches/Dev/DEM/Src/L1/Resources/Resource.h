#pragma once
#ifndef __DEM_L1_RESOURCE_H__
#define __DEM_L1_RESOURCE_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Resource wraps a piece of abstract data, used by other systems. It can be loaded and managed by
// resource system, so clients needn't to care about loading, unloading and LOD of resources.

namespace Resources
{
class IResourceManager;
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

	CStrID				UID;
	EResourceState		State;
	IResourceManager*	pManager;

	CResource(CStrID ID, IResourceManager* pHost): UID(ID), pManager(pHost), State(Rsrc_NotLoaded) {} // Create only by manager

public:

	virtual ~CResource() {}

	bool			Load(LPCSTR pFileName, CResourceLoader* pLoader); //???or request mgr to load?
	bool			Reload();
	virtual void	Unload() { State = Rsrc_NotLoaded; }

	CStrID			GetUID() const { return UID; }
	EResourceState	GetState() const { return State; }
	bool			IsLoaded() const { return State == Rsrc_Loaded; }
};

typedef Ptr<CResource> PResource;

}

#endif
