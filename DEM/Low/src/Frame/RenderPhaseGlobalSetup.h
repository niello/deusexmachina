#pragma once
#ifndef __DEM_L1_FRAME_PHASE_GLOBAL_SETUP_H__
#define __DEM_L1_FRAME_PHASE_GLOBAL_SETUP_H__

#include <Frame/RenderPhase.h>

// Performs setup of global shader variables and other frame-wide params.

namespace Render
{
	struct CEffectConstant;
}

namespace Frame
{

class CRenderPhaseGlobalSetup: public CRenderPhase
{
	__DeclareClass(CRenderPhaseGlobalSetup);

protected:

	// Global shader params
	const Render::CEffectConstant* pConstViewProjection;
	const Render::CEffectConstant* pConstCameraPosition;

public:

	//virtual ~CRenderPhaseGlobalSetup() {}

	virtual bool Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc);
	virtual bool Render(CView& View);
};

}

#endif
