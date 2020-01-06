#pragma once
#include <Render/Renderer.h>
#include <Render/VertexComponent.h>
#include <Render/SamplerDesc.h>
#include <Data/FixedArray.h>
#include <Data/Ptr.h>

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

	static const U32	INSTANCE_BUFFER_STREAM_INDEX = 1;
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

	struct CPatchInstance
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

	struct CProcessTerrainNodeArgs
	{
		const CCDLODData*		pCDLOD;
		const CRenderContext*	pRenderContext;
		CPatchInstance*			pInstances;
		float*					pMorphConsts;
		UPTR					MaxInstanceCount;
		float					AABBMinX;
		float					AABBMinZ;
		float					ScaleBaseX;
		float					ScaleBaseZ;
		U16						LightIndexBase;
		U8						LightCount;
	};

	CSamplerDesc							HMSamplerDesc;
	PSampler								HMSampler;			//!!!binds an RP to a specific GPU!

	UPTR									CurrMaxLightCount = 0;
	CFixedArray<CVertexComponent>			InstanceDataDecl;
	CDict<CVertexLayout*, PVertexLayout>	InstancedLayouts;	//!!!duplicate in different instances of the same renderer!
	PVertexBuffer							InstanceVB;			//!!!binds an RP to a specific GPU!
	CPatchInstance*							pInstances = nullptr;
	UPTR									MaxInstanceCount;	//???where to define? in a phase? or some setting? or move to CView with a VB?

	static ENodeStatus	ProcessTerrainNode(const CProcessTerrainNodeArgs& Args, U32 X, U32 Z, U32 LOD, float LODRange, U32& PatchCount, U32& QPatchCount, U8& MaxLightCount, EClipStatus Clip = Clipped);
	static bool			CheckNodeSphereIntersection(const CLightTestArgs& Args, const sphere& Sphere, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter);
	static bool			CheckNodeFrustumIntersection(const CLightTestArgs& Args, const matrix44& Frustum, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter);
	static void			FillNodeLightIndices(const CProcessTerrainNodeArgs& Args, CPatchInstance& Patch, const CAABB& NodeAABB, U8& MaxLightCount);

public:

	CTerrainRenderer();
	virtual ~CTerrainRenderer() override;

	virtual bool                 Init(bool LightingEnabled) override;
	virtual bool                 PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context) override;
	virtual CRenderQueueIterator Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr) override;
};

}
