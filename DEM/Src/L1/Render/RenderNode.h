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
class CMaterial;
class CTechnique;

struct CRenderNode
{
	IRenderable*		pRenderable; //???PRenderable?
	IRenderer*			pRenderer;
	//float				SqDistanceToCamera; / OnScreenSize
	matrix44			Transform;
	const matrix44*		pSkinPalette;	// NULL if no skin
	UPTR				BoneCount;		// Undefined if no skin
	//UPTR				LOD; // (selected by a camera distance or a screen size, if available - can project AABB)
	//geometry (selected by LOD) - or get in renderer?
	const CMaterial*	pMaterial;		// Chosen by LOD
	const CTechnique*	pTech;			// Chosen by material, renderable and renderer
	//light indices, if lighting enabled
};

}

#endif
