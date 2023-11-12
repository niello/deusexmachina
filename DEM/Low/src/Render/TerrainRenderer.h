#pragma once
#include <Render/Renderer.h>
#include <Render/ShaderParamStorage.h>
#include <map>

// Default renderer for CTerrain render objects.
// Currently supports only scaling and translation.

class sphere;

namespace Render
{

class CTerrainRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	struct CTerrainTechInterface
	{
		CShaderParamStorage  PerInstanceParams;

		CShaderConstantParam ConstVSCDLODParams;
		CShaderConstantParam ConstGridConsts;
		CShaderConstantParam ConstFirstInstanceIndex;
		CShaderConstantParam ConstInstanceDataVS;
		CShaderConstantParam ConstInstanceDataPS;
		CShaderConstantParam MemberLightIndices;
		PResourceParam       ResourceHeightMap;
		PSamplerParam        VSLinearSampler;

		UPTR TechMaxInstanceCount = 1;
		U32  TechLightCount = 0;
		bool TechNeedsMaterial = false;
	};

	const CTechnique*      _pCurrTech = nullptr;
	CTerrainTechInterface* _pCurrTechInterface = nullptr;
	const CMaterial*       _pCurrMaterial = nullptr;
	CGPUDriver*            _pGPU = nullptr;

	PSampler               _HeightMapSampler;

	std::map<const CTechnique*, CTerrainTechInterface> _TechInterfaces;

	CTerrainTechInterface* GetTechInterface(const CTechnique* pTech);

public:

	CTerrainRenderer();
	virtual ~CTerrainRenderer() override;

	virtual bool Init(const Data::CParams& Params, CGPUDriver& GPU) override;
	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable) override;
	virtual void EndRange(const CRenderContext& Context) override {}
};

}
