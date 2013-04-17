#pragma once
#ifndef __DEM_L1_SCENE_MODEL_H__
#define __DEM_L1_SCENE_MODEL_H__

#include <Scene/RenderObject.h>
#include <Render/Materials/Material.h>
#include <Render/Geometry/Mesh.h>
#include <util/nfixedarray.h>

// Mesh is a scene node attribute representing a visible shape.
// Mesh attribute references VB & IB resources, stores vertex and index range,
// material, its parameters and texture refs.

//!!!it is good to have attr for each mesh group, so separate visibility test
// is performed and shader sorting simplifies

class bbox3;

namespace Scene
{
struct CSPSRecord;

class CModel: public CRenderObject
{
	DeclareRTTI;
	DeclareFactory(CModel);

public:

	Render::PMesh			Mesh;
	DWORD					MeshGroupIndex;
	Render::PMaterial		Material;
	DWORD					FeatureFlags;	// Model shader flags like Skinned, must be ORed with material flags before use
	Render::CShaderVarMap	ShaderVars;		// Animable per-object vars, also can store geom. vars like CullMode
	CStrID					BatchType; //???use in CRenderObject and don't check RTTI at all?
	nFixedArray<int>		BoneIndices;	// For skinning splits due to shader constants limit only

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling
	//can use Flags field of CSceneNodeAttr

	CSPSRecord*				pSPSRecord;

	CModel(): pSPSRecord(NULL), MeshGroupIndex(0), FeatureFlags(0) {}

	virtual bool	LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);
	virtual bool	OnAdd();
	virtual void	OnRemove();
	virtual void	Update();

	const bbox3&	GetLocalAABB() const { return Mesh->GetGroup(MeshGroupIndex).AABB; }
	void			GetGlobalAABB(bbox3& OutBox) const;
};

RegisterFactory(CModel);

typedef Ptr<CModel> PModel;

}

#endif
