#pragma once
#include <Render/Renderer.h>
#include <Render/ShaderParamStorage.h>
#include <map>

// Default renderer for CModel render objects.
// Implements "Model" and "ModelSkinned" input sets.

namespace Render
{

class CModelRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	struct CModelTechInterface
	{
		CShaderParamStorage  PerInstanceParams;

		CShaderConstantParam ConstInstanceData;
		CShaderConstantParam ConstSkinPalette;
		CShaderConstantParam MemberWorldMatrix;
		CShaderConstantParam MemberFirstBoneIndex;
		CShaderConstantParam MemberLightIndices;

		UPTR TechMaxInstanceCount = 1;
		U32  TechLightCount = 0;
		bool TechNeedsMaterial = false;
	};

	const CTechnique* _pCurrTech = nullptr;
	CModelTechInterface* _pCurrTechInterface = nullptr;
	const CMaterial* _pCurrMaterial = nullptr;
	const CMesh* _pCurrMesh = nullptr;
	const CPrimitiveGroup* _pCurrGroup = nullptr;
	UPTR _BufferedBoneCount = 0;
	UPTR _InstanceCount = 0;

	std::map<const CTechnique*, CModelTechInterface> _TechInterfaces;
	std::vector<U32> _LightIndexBuffer; // here to avoid per frame reallocation

	CModelTechInterface* GetTechInterface(const CTechnique* pTech, CGPUDriver& GPU);
	void CommitCollectedInstances(CGPUDriver& GPU);

public:

	virtual bool Init(const Data::CParams& Params, CGPUDriver& GPU) override { OK; }

	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable, IRenderModifier* pModifier = nullptr) override;
	virtual void EndRange(const CRenderContext& Context) override;
};

}
