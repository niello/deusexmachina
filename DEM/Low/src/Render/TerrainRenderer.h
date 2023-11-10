#pragma once
#include <Render/Renderer.h>
#include <Render/VertexComponent.h>
#include <Render/SamplerDesc.h>
#include <Render/ShaderParamTable.h>
#include <Data/FixedArray.h>
#include <Data/Ptr.h>
#include <map>

// Default renderer for CTerrain render objects.
// Currently supports only the translation part of the transformation.

class sphere;

namespace Render
{
class CTerrain;
class CCDLODData;

class CTerrainRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

protected:

	enum ENodeStatus
	{
		Node_Invisible,
		Node_NotInLOD,
		Node_Processed
	};

	PSampler HeightMapSampler;

	const CMaterial* pCurrMaterial = nullptr;
	const CTechnique* pCurrTech = nullptr;

	CShaderConstantParam ConstVSCDLODParams;
	CShaderConstantParam ConstGridConsts;
	CShaderConstantParam ConstFirstInstanceIndex;
	CShaderConstantParam ConstInstanceDataVS;
	CShaderConstantParam ConstInstanceDataPS;
	PResourceParam ResourceHeightMap;

	// Subsequent shader constants for single-instance case
	CShaderConstantParam ConstWorldMatrix;
	CShaderConstantParam ConstLightCount;
	CShaderConstantParam ConstLightIndices;

public:

	CTerrainRenderer();
	virtual ~CTerrainRenderer() override;

	virtual bool Init(const Data::CParams& Params, CGPUDriver& GPU) override;
	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable) override;
	virtual void EndRange(const CRenderContext& Context) override {}
};

}
