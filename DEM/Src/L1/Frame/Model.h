#pragma once
#ifndef __DEM_L1_FRAME_MODEL_H__
#define __DEM_L1_FRAME_MODEL_H__

#include <Frame/RenderObject.h>
#include <Data/FixedArray.h>

// Model represents a piece of visible polygonal geometry in a scene.
// It ties together a mesh group, a material and per-object rendering params.

class CAABB;

namespace Scene
{
	struct CSPSRecord;
}

namespace Render
{
	typedef Ptr<class CMesh> PMesh;
	typedef Ptr<class CMaterial> PMaterial;
}

namespace Frame
{

class CModel: public CRenderObject
{
	__DeclareClass(CModel);

protected:

	Scene::CSPSRecord*	pSPSRecord;

	virtual void	OnDetachFromNode();
	virtual bool	ValidateResources();

public:

	Render::PMesh		Mesh;
	UPTR				MeshGroupIndex;
	Render::PMaterial	Material; //???!!!materialset!?
	U32					FeatureFlags;	// Model shader flags like Skinned, must be ORed with material flags before use
//	CShaderVarMap		ShaderVars;		// Animable per-object vars
	CStrID				BatchType;	//???use in CRenderObject and don't check RTTI at all?
	CFixedArray<int>	BoneIndices;	// For skinning splits due to shader constants limit only

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling (Skinned, EnableInstancing etc too?)
	//can use Flags field of CNodeAttribute

	CModel(): pSPSRecord(NULL), MeshGroupIndex(0), FeatureFlags(0) {}

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);

	virtual void	UpdateInSPS(Scene::CSPS& SPS);
	const CAABB&	GetLocalAABB(UPTR LOD = 0) const;
	void			GetGlobalAABB(CAABB& OutBox, UPTR LOD = 0) const;
};

typedef Ptr<CModel> PModel;

}

#endif
