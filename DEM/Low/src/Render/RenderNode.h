#pragma once
#include <Data/RefCounted.h>
#include <Render/RenderFwd.h>
#include <Math/Matrix44.h>
#include <map>

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

	CMaterial*              pMaterial;          // Non-constant because of temporary buffers in a shader param storage
	const CTechnique*		pTech;
	const CMesh*			pMesh;
	const CPrimitiveGroup*	pGroup;

	U16						LightIndexBase;		// Memory is actually allocated inside a CView, we store index, not ptr, to handle reallocations
	U8						LightCount;			// If zero, LightIndexBase is undefined
};

struct CLightRecord
{
	const Render::CLight_OLD_DELETE*	pLight;
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

	std::map<Render::EEffectType, Render::PEffect> EffectOverrides;
	const Render::CTechnique* const* pShaderTechCache = nullptr; //!!!DBG TMP!
};

}
