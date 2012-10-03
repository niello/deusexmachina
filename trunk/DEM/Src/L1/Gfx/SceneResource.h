#pragma once
#ifndef __DEM_L1_SCENE_RESOURCE_H__
#define __DEM_L1_SCENE_RESOURCE_H__

#include <Core/RefCounted.h>
#include <kernel/nref.h>
#include <kernel/nroot.h>

// Wraps a single Nebula graphics object identified by a resource name.
// The resource name must consist of a category, a name and an optional
// subname, delimited by slashes.

class nTransformNode;
class nRenderContext;

namespace Graphics
{

class CSceneResource: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CSceneResource);

private:

	nRef<nRoot>				refRootNode;	// need to keep track of root node for proper refcounting
	nRef<nTransformNode>	refNode;

public:

	nString					Name;

	~CSceneResource() { if (IsLoaded()) Unload(); }

	void			Load();
	void			Unload();

	bool			IsLoaded() const { return refNode.isvalid(); }
	nTransformNode*	GetNode() const { return refNode.get(); }
};

RegisterFactory(CSceneResource);

typedef Ptr<CSceneResource> PSceneResource;

}

#endif
