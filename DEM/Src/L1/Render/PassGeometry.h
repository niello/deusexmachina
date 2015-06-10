#pragma once
#ifndef __DEM_L1_RENDER_PASS_GEOMETRY_H__
#define __DEM_L1_RENDER_PASS_GEOMETRY_H__

#include <Render/RenderPhase.h>
#include <Render/Renderer.h>
#include <Data/Dictionary.h>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Render
{

class CPassGeometry: public CRenderPhase
{
protected:

	CArray<PRenderer>	BatchRenderers;

// Input:
// Geometry, lights (if lighting is enabled)
// Output:
// Mid-RT or frame RT (backbuffer)

// Batch has batch shader (common), shader vars
// shader feature flags of the batch + batch type + mesh filter
// lighting type (some geoms are rendered unlit)
// sorting type

public:

	virtual bool Init(CStrID PassName, const Data::CParams& Desc, const CDict<CStrID, PRenderTarget>& RenderTargets);
	virtual void Render(const CArray<CRenderObject*>* pObjects, const CArray<CLight*>* pLights);
};

}

#endif
