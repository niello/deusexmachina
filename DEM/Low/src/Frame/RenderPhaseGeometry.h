#pragma once
#include <Frame/RenderPhase.h>
#include <Render/ShaderParamTable.h>
#include <Data/Dictionary.h>
#include <Data/FixedArray.h>
#include <map>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Render
{
	using PRenderer = std::unique_ptr<class IRenderer>;
}

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

	ESortingType									 SortingType;
	CFixedArray<CStrID>								 RenderTargetIDs;
	CStrID											 DepthStencilID;
	std::vector<Render::PRenderer>                   Renderers;
	std::map<const Core::CRTTI*, Render::IRenderer*> RenderersByObjectType;
	std::map<Render::EEffectType, CStrID>			 EffectOverrides;
	bool											 EnableLighting = false;

	Render::CShaderConstantParam					 ConstGlobalLightBuffer;
	Render::PResourceParam							 RsrcIrradianceMap;
	Render::PResourceParam							 RsrcRadianceEnvMap;
	Render::PSamplerParam							 SampTrilinearCube;

public:

	CRenderPhaseGeometry();
	virtual ~CRenderPhaseGeometry() override;

	virtual bool Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) override;
	virtual bool Render(CView& View) override;
};

}
