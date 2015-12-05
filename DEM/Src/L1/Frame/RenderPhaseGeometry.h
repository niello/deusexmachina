#pragma once
#ifndef __DEM_L1_FRAME_PHASE_GEOMETRY_H__
#define __DEM_L1_FRAME_PHASE_GEOMETRY_H__

#include <Frame/RenderPhase.h>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Frame
{

class CRenderPhaseGeometry: public CRenderPhase
{
protected:

	//CArray<PRenderer>	BatchRenderers;

// Input:
// Geometry, lights (if lighting is enabled)
// Output:
// Intermediate RT or swap chain RT (backbuffer), DSV

public:

	//virtual bool Init(CStrID PassName, const Data::CParams& Desc, const CDict<CStrID, PRenderTarget>& RenderTargets);
	//virtual void Render(const CArray<CRenderObject*>* pObjects, const CArray<CLight*>* pLights);
};

}

#endif
