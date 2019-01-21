#pragma once
#ifndef __DEM_L1_FRAME_PHASE_GEOMETRY_H__
#define __DEM_L1_FRAME_PHASE_GEOMETRY_H__

#include <Frame/RenderPhase.h>
#include <Render/RenderFwd.h>
#include <Data/Dictionary.h>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Frame
{

class CRenderPhaseGeometry: public CRenderPhase
{
	__DeclareClass(CRenderPhaseGeometry);

protected:

	enum ESortingType
	{
		Sort_None,
		Sort_FrontToBack,
		Sort_Material
	};

	ESortingType									SortingType;
	CFixedArray<I32>								RenderTargetIndices;
	I32												DepthStencilIndex;
	CDict<const Core::CRTTI*, Render::IRenderer*>	Renderers;
	Render::CEffectOverrideMap						EffectOverrides;
	bool											EnableLighting = false;

	const Render::CEffectConstant*					pConstGlobalLightBuffer = nullptr;
	const Render::CEffectResource*					pRsrcIrradianceMap = nullptr;
	const Render::CEffectResource*					pRsrcRadianceEnvMap = nullptr;
	const Render::CEffectSampler*					pSampTrilinearCube = nullptr;

public:

	CRenderPhaseGeometry();
	virtual ~CRenderPhaseGeometry();

	virtual bool Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc);
	virtual bool Render(CView& View);
};

}

#endif
