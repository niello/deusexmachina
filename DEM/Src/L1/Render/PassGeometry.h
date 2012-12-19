#pragma once
#ifndef __DEM_L1_RENDER_PASS_GEOMETRY_H__
#define __DEM_L1_RENDER_PASS_GEOMETRY_H__

#include <Render/Pass.h>
#include <Render/Renderer.h>

//!!!OLD!
#include "renderpath/nrpphase.h"

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Render
{

class CPassGeometry: public CPass
{
	//DeclareRTTI;

//protected:
public:

	nArray<PRenderer>	BatchRenderers;

// Input:
// Geometry, lights (if lighting is enabled)
// Output:
// Mid-RT or frame RT

// Batch has batch shader (common), shader vars
// shader feature flags of the batch + batch type + mesh filter
// lighting type (some geoms are rendered unlit)
// sorting type

//!!!OLD!
	nArray<nRpPhase> phases;

public:

	virtual bool Init(CStrID PassName, const Data::CParams& Desc, const nDictionary<CStrID, PRenderTarget>& RenderTargets);
	virtual void Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights);

	//!!!OLD!
	virtual void Validate();
};

//typedef Ptr<CPass> PPass;

}

#endif
