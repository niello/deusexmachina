#pragma once
#ifndef __DEM_L1_RENDER_NODE_H__
#define __DEM_L1_RENDER_NODE_H__

#include <Data/RefCounted.h>
#include <Render/RenderFwd.h>
#include <Math/Matrix44.h>

// An element of a render queue or tree. Caches some values for faster sorting and rendering.
// Render node context encapsulates external data that shouldn't be stored in a render node
// itself, but is important for its preparation by renderers.

namespace Render
{

struct CRenderNode
{
	matrix44				Transform;
	IRenderable*			pRenderable;
	IRenderer*				pRenderer;

	UPTR					Order;				// Rendering order, used to sort by alpha etc
	float					SqDistanceToCamera;

	const CMaterial*		pMaterial;
	const CEffect*			pEffect;			// For searching instanced tech in a model renderer only (for now)
	const CTechnique*		pTech;
	const CMesh*			pMesh;
	const CPrimitiveGroup*	pGroup;

	const matrix44*			pSkinPalette;		// nullptr if no skin
	const int*				pSkinMapping;		// If nullptr, no remapping needed
	UPTR					BoneCount;			// Undefined if no skin

	U16						LightIndexBase;		// Memory is actually allocated inside a CView, we store index, not ptr, to handle reallocations
	U8						LightCount;			// If zero, LightIndexBase is undefined
};

struct CLightRecord
{
	const Render::CLight*	pLight;
	matrix44				Transform;
	UPTR					UseCount;
	IPTR					GPULightIndex;
};

struct CRenderNodeContext
{
	UPTR					MeshLOD;
	UPTR					MaterialLOD;
	CAABB					AABB;
	CArray<CLightRecord>*	pLights;
	CArray<U16>*			pLightIndices;

	CDict<Render::EEffectType, Render::PEffect>	EffectOverrides;
};

}

#endif
