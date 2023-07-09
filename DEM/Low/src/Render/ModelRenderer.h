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

	static const U32						INSTANCE_BUFFER_STREAM_INDEX = 1;
	static const U16						MAX_LIGHT_COUNT_PER_OBJECT = 8; //???to setting?

	CFixedArray<CVertexComponent>           InstanceDataDecl;
	std::map<CVertexLayout*, PVertexLayout> InstancedLayouts;	//!!!duplicate in different instances of the same renderer!
	PVertexBuffer                           InstanceVB;        //!!!binds an RP to a specific GPU!
	UPTR                                    InstanceVBSize = 0;

	const CMaterial* pCurrMaterial = nullptr;
	const CMesh* pCurrMesh = nullptr;
	const CTechnique* pCurrTech = nullptr;
	CVertexLayout* pVL = nullptr;
	CVertexLayout* pVLInstanced = nullptr;

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
