#pragma once
#ifndef __DEM_L1_FRAME_RENDER_NODE_H__
#define __DEM_L1_FRAME_RENDER_NODE_H__

#include <Data/RefCounted.h>
//#include <System/Allocators/PoolAllocator.h>

// An element of a render queue or tree

//!!!if will be decoupled from Scene::, can move renderers and render nodes to a Render:: instead of Frame::

namespace Frame
{
typedef Ptr<class CRenderObject> PRenderObject;
class IRenderer;

struct CRenderNode
{
	PRenderObject	RenderObject;
	IRenderer*		pRenderer;
	//float			SqDistanceToCamera;
	//matrix44		Transform; // for Scene/Render decoupling, but then must decouple CRenderObject first
	//UPTR			LOD; // (selected by a camera distance or a screen size, if available - can project AABB)
	//material (selected by LOD)
	//geometry (selected by LOD) - or get in renderer?
	//effect technique (selected by material and geometry)
	//light indices, if lighting enabled
};

}

#endif
