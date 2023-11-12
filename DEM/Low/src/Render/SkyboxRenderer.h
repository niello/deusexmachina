#pragma once
#include <Render/Renderer.h>
#include <Render/ShaderParamStorage.h>
#include <map>

// Default renderer for CSkybox render objects.

namespace Render
{
class CMaterial;
class CTechnique;

class CSkyboxRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	struct CSkyboxTechInterface
	{
		CShaderParamStorage  PerInstanceParams;
		CShaderConstantParam ConstWorldMatrix;
	};

	const CTechnique*     _pCurrTech = nullptr;
	CSkyboxTechInterface* _pCurrTechInterface = nullptr;
	const CMaterial*      _pCurrMaterial = nullptr;
	CGPUDriver*           _pGPU = nullptr;

	std::map<const CTechnique*, CSkyboxTechInterface> _TechInterfaces;

	CSkyboxTechInterface* GetTechInterface(const CTechnique* pTech);

public:

	CSkyboxRenderer();

	virtual bool Init(const Data::CParams& Params, CGPUDriver& GPU) override { OK; }
	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable) override;
	virtual void EndRange(const CRenderContext& Context) override;
};

}
