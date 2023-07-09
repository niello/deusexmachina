#pragma once
#include <Render/Renderer.h>
#include <Render/ShaderParamTable.h>

// Default renderer for CSkybox render objects.

namespace Render
{
class CMaterial;
class CTechnique;

class CSkyboxRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	const CMaterial*     _pCurrMaterial = nullptr;
	const CTechnique*    _pCurrTech = nullptr;
	CShaderConstantParam _ConstWorldMatrix;

public:

	CSkyboxRenderer();

	virtual bool Init(bool LightingEnabled, const Data::CParams& Params) override { OK; }
	virtual bool PrepareNode(IRenderable& Node, const CRenderNodeContext& Context) override;

	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/) override;
	virtual void EndRange(const CRenderContext& Context) override;
};

}
