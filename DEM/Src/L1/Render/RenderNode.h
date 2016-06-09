#pragma once
#ifndef __DEM_L1_RENDER_NODE_H__
#define __DEM_L1_RENDER_NODE_H__

#include <Data/RefCounted.h>
#include <Math/Matrix44.h>
//#include <System/Allocators/PoolAllocator.h>

// An element of a render queue or tree. Caches some values for faster sorting and rendering.

namespace Render
{
class IRenderable;
class IRenderer;
class CMaterial;
class CTechnique;
struct CPrimitiveGroup;

struct CRenderNode
{
	matrix44				Transform;
	IRenderable*			pRenderable; //???PRenderable?
	IRenderer*				pRenderer;
	const matrix44*			pSkinPalette;	// NULL if no skin
	UPTR					BoneCount;		// Undefined if no skin
	const CPrimitiveGroup*	pGroup;
	const CMaterial*		pMaterial;
	const CTechnique*		pTech;
	float					SqDistanceToCamera;
	//light indices, if lighting enabled
};

}

#endif
