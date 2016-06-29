#pragma once
#ifndef __DEM_L1_RENDER_NODE_H__
#define __DEM_L1_RENDER_NODE_H__

#include <Data/RefCounted.h>
#include <Render/RenderFwd.h>
#include <Math/Matrix44.h>
//#include <System/Allocators/PoolAllocator.h>

// An element of a render queue or tree. Caches some values for faster sorting and rendering.
// Render node context encapsulates external data that shouldn't be stored in a render node
// itself, but is important for its preparation by renderers.

namespace Render
{

struct CRenderNode
{
	matrix44				Transform;
	IRenderable*			pRenderable; //???PRenderable?
	IRenderer*				pRenderer;
	const matrix44*			pSkinPalette;	// NULL if no skin
	UPTR					BoneCount;		// Undefined if no skin
	UPTR					Order;			// Rendering order, used to sort by alpha etc
	const CMaterial*		pMaterial;
	const CTechnique*		pTech;
	const CMesh*			pMesh;
	const CPrimitiveGroup*	pGroup;
	float					SqDistanceToCamera;
	//light indices, if lighting enabled
};

struct CRenderNodeContext
{
	UPTR					MeshLOD;
	UPTR					MaterialLOD;
	CMaterialMap*			pMaterialOverrides;
};

}

#endif
