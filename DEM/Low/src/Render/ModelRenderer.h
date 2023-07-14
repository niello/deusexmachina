#pragma once
#include <Render/Renderer.h>
#include <Render/ShaderParamStorage.h>

// Default renderer for CModel render objects.
// Implements "Model" and "ModelSkinned" input sets.

// TODO: describe input sets!!!

namespace Render
{

class CModelRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	const CTechnique* _pCurrTech = nullptr;
	const CMaterial* _pCurrMaterial = nullptr;
	const CMesh* _pCurrMesh = nullptr;
	const CPrimitiveGroup* _pCurrGroup = nullptr;
	CGPUDriver* _pGPU = nullptr;
	UPTR _BufferedBoneCount = 0;
	UPTR _InstanceCount = 0;
	UPTR _TechMaxInstanceCount = 1;
	bool _TechNeedsMaterial = false;

	CShaderParamStorage  _PerInstance;

	CShaderConstantParam _ConstInstanceData;
	CShaderConstantParam _ConstSkinPalette;
	CShaderConstantParam _MemberWorldMatrix;
	CShaderConstantParam _MemberFirstBoneIndex;
	CShaderConstantParam _MemberLightCount;
	CShaderConstantParam _MemberLightIndices;

	void CommitCollectedInstances();

public:

	virtual bool Init(bool LightingEnabled, const Data::CParams& Params) override;
	virtual bool PrepareNode(IRenderable& Node, const CRenderNodeContext& Context) override;

	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/) override;
	virtual void EndRange(const CRenderContext& Context) override;
};

}
