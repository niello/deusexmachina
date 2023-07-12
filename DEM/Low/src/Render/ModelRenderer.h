#pragma once
#include <Render/Renderer.h>
#include <Render/VertexComponent.h>
#include <Render/ShaderParamTable.h>
#include <Data/FixedArray.h>
#include <Data/Ptr.h>
#include <map>

// Default renderer for CModel render objects

namespace Render
{
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;

class CModelRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	const CMaterial* _pCurrMaterial = nullptr;
	const CMesh* _pCurrMesh = nullptr;
	const CTechnique* _pCurrTech = nullptr;
	CVertexLayout* pVL = nullptr;
	CVertexLayout* pVLInstanced = nullptr;
	bool _TechNeedsMaterial = false;

	// Effect constants
	CShaderConstantParam ConstInstanceDataVS; // Model, ModelSkinned, ModelInstanced
	CShaderConstantParam ConstInstanceDataPS; // Model, ModelSkinned, ModelInstanced
	CShaderConstantParam ConstSkinPalette;    // ModelSkinned

	// Subsequent shader constants for single-instance case
	CShaderConstantParam ConstWorldMatrix;
	CShaderConstantParam ConstLightCount;
	CShaderConstantParam ConstLightIndices;

public:

	virtual bool Init(bool LightingEnabled, const Data::CParams& Params) override;
	virtual bool PrepareNode(IRenderable& Node, const CRenderNodeContext& Context) override;

	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/) override;
	virtual void EndRange(const CRenderContext& Context) override;
};

}
