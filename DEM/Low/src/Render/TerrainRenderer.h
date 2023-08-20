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

	static const U32	INSTANCE_MAX_LIGHT_COUNT = 7;
	//static const U32	INSTANCE_PADDING_SIZE = (7 - INSTANCE_MAX_LIGHT_COUNT % 8); // To preserve align-16 compatible structure size

	// Controls how much AABB tests per light are allowed at the coarse lighting test.
	// Light intersection recurse through CDLOD quadtree until the light is thrown out,
	// all quadtree nodes are tested or this value is reached. When it is reached, all
	// other possibly touching lights are added to be processed in the narrow phase.
	// Actual test count may slightly exceed from this value.
	static const UPTR	LIGHT_INTERSECTION_COARSE_TEST_MAX_AABBS = 64;

	enum ENodeStatus
	{
		Node_Invisible,
		Node_NotInLOD,
		Node_Processed
	};

	struct alignas(16) CPatchInstance
	{
		// Vertex shader instance data
		float	ScaleOffset[4];
		float	MorphConsts[2];
		//float	_PAD1[2];								// To preserve align-16 of PS data

		// Pixel shader instance data
		I16		LightIndex[INSTANCE_MAX_LIGHT_COUNT]; // + INSTANCE_PADDING_SIZE];
	};

	struct CLightTestArgs
	{
		const CCDLODData*	pCDLOD;
		float				AABBMinX;
		float				AABBMinZ;
		float				ScaleBaseX;
		float				ScaleBaseZ;
	};

	PSampler HeightMapSampler;
	CPatchInstance* pInstances = nullptr;
	U32 MaxInstanceCount = 0;	//???where to define? in a phase? or some setting? or move to CView with a VB?

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

	static bool			CheckNodeSphereIntersection(const CLightTestArgs& Args, const sphere& Sphere, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter);
	static bool			CheckNodeFrustumIntersection(const CLightTestArgs& Args, const matrix44& Frustum, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter);

public:

	CTerrainRenderer();
	virtual ~CTerrainRenderer() override;

	virtual bool Init(const Data::CParams& Params, CGPUDriver& GPU) override;
	virtual bool BeginRange(const CRenderContext& Context) override;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable) override;
	virtual void EndRange(const CRenderContext& Context) override;
};

}
