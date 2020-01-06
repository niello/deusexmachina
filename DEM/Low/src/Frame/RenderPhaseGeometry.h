#pragma once
#include <Frame/RenderPhase.h>
#include <Render/ShaderParamTable.h>
#include <Data/Dictionary.h>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Frame
{

class CRenderPhaseGeometry: public CRenderPhase
{
	FACTORY_CLASS_DECL;

protected:

	enum ESortingType
	{
		Sort_None,
		Sort_FrontToBack,
		Sort_Material
	};

	ESortingType									SortingType;
	CFixedArray<CStrID>								RenderTargetIDs;
	CStrID											DepthStencilID;
	CDict<const Core::CRTTI*, Render::IRenderer*>	Renderers;
	CDict<Render::EEffectType, CStrID>				EffectOverrides;
	bool											EnableLighting = false;

	Render::CShaderConstantParam					ConstGlobalLightBuffer;
	Render::PResourceParam							RsrcIrradianceMap;
	Render::PResourceParam							RsrcRadianceEnvMap;
	Render::PSamplerParam							SampTrilinearCube;

public:

	CRenderPhaseGeometry();
	virtual ~CRenderPhaseGeometry() override;

	virtual bool Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc);
	virtual bool Render(CView& View);
};

}
