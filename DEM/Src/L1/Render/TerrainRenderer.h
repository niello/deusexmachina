#pragma once
#ifndef __DEM_L1_RENDER_TERRAIN_RENDERER_H__
#define __DEM_L1_RENDER_TERRAIN_RENDERER_H__

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
	__DeclareClass(CTerrainRenderer);

protected:

	enum ENodeStatus
	{
		Node_Invisible,
		Node_NotInLOD,
		Node_Processed
	};

	struct CPatchInstance
	{
		float ScaleOffset[4];
		float MorphConsts[2];
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
		const CCDLODData*	pCDLOD;
		CPatchInstance*		pInstances;
		float*				pMorphConsts;
		const matrix44*		pViewProj;
		const vector3*		pCameraPos;
		UPTR				MaxInstanceCount;
		float				AABBMinX;
		float				AABBMinZ;
		float				ScaleBaseX;
		float				ScaleBaseZ;
	};

	static const U32						INSTANCE_BUFFER_STREAM_INDEX = 1;

	// Controls how much AABB tests per light are allowed at the coarse lighting test.
	// Light intersection recurse through CDLOD quadtree until the light is thrown out,
	// all quadtree nodes are tested or this value is reached. When it is reached, all
	// other possibly touching lights are added to be processed in the narrow phase.
	// Actual test count may slightly exceed from this value.
	static const UPTR						LIGHT_INTERSECTION_COARSE_TEST_MAX_AABBS = 64;

	UPTR									InputSet_CDLOD;

	CSamplerDesc							HMSamplerDesc;
	PSampler								HMSampler;			//!!!binds an RP to a specific GPU!

	CFixedArray<CVertexComponent>			InstanceDataDecl;
	CDict<CVertexLayout*, PVertexLayout>	InstancedLayouts;	//!!!duplicate in different instances of the same renderer!
	PVertexBuffer							InstanceVB;			//!!!binds an RP to a specific GPU!
	CPatchInstance*							pInstances;			// CPU mirror of GPU buffer, used to prepare data and pass into GPU in one shot
	UPTR									MaxInstanceCount;	//???where to define? in a phase? or some setting? or move to CView with a VB?

	static ENodeStatus	ProcessTerrainNode(const CProcessTerrainNodeArgs& Args, U32 X, U32 Z, U32 LOD, float LODRange, U32& PatchCount, U32& QPatchCount, EClipStatus Clip = Clipped);
	static bool			CheckNodeSphereIntersection(const CLightTestArgs& Args, const sphere& Sphere, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter);
	static bool			CheckNodeFrustumIntersection(const CLightTestArgs& Args, const matrix44& Frustum, U32 X, U32 Z, U32 LOD, UPTR& AABBTestCounter);

public:

	CTerrainRenderer();
	~CTerrainRenderer();

	virtual bool							PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context);
	virtual CArray<CRenderNode*>::CIterator	Render(const CRenderContext& Context, CArray<CRenderNode*>& RenderQueue, CArray<CRenderNode*>::CIterator ItCurr);
};

}

#endif
