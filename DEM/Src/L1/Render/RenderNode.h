#pragma once
#ifndef __DEM_L1_RENDER_NODE_H__
#define __DEM_L1_RENDER_NODE_H__

#include <Data/RefCounted.h>
#include <Math/Matrix44.h>
//#include <System/Allocators/PoolAllocator.h>

// An element of a render queue or tree

//!!!if will be decoupled from Scene::, can move renderers and render nodes to a Render:: instead of Frame::

namespace Render
{
class IRenderable;
class IRenderer;

struct CRenderNode
{
	IRenderable*	pRenderable; //???PRenderable?
	IRenderer*		pRenderer;
	//float			SqDistanceToCamera;
	matrix44		Transform;
	//UPTR			LOD; // (selected by a camera distance or a screen size, if available - can project AABB)
	//skin data
	//material (selected by LOD)
	//geometry (selected by LOD) - or get in renderer?
	//effect technique (selected by material and geometry)
	//light indices, if lighting enabled
};

}

#endif
