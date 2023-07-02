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
	const Render::CTechnique* const* pShaderTechCache = nullptr;
};

}
