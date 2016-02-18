#pragma once
#ifndef __DEM_L1_FRAME_PHASE_GEOMETRY_H__
#define __DEM_L1_FRAME_PHASE_GEOMETRY_H__

#include <Frame/RenderPhase.h>
#include <Data/Dictionary.h>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Render
{
	class IRenderer;
}

namespace Frame
{

class CRenderPhaseGeometry: public CRenderPhase
{
	__DeclareClass(CRenderPhaseGeometry);

protected:

	CDict<const Core::CRTTI*, Render::IRenderer*> Renderers;

public:

	//virtual ~CRenderPhaseGeometry() {}

	virtual bool Init(CStrID PhaseName, const Data::CParams& Desc);
	virtual bool Render(CView& View);
};

}

#endif
