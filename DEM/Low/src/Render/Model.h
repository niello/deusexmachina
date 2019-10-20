#pragma once
#include <Render/Renderable.h>
#include <Data/FixedArray.h>
#include <Data/StringID.h>

// Model represents a piece of visible polygonal geometry.
// It ties together a mesh group, a material and per-object rendering params.

namespace Render
{
typedef Ptr<class CMesh> PMesh;
typedef Ptr<class CMaterial> PMaterial;

class CModel: public IRenderable
{
	__DeclareClass(CModel);

protected:

	virtual bool	ValidateResources(CGPUDriver* pGPU);

public:

	CStrID				MeshUID;
	PMesh				Mesh;
	UPTR				MeshGroupIndex = 0;
	CStrID				MaterialUID;
	PMaterial			Material; //???!!!materialset!?
	CFixedArray<int>	BoneIndices;	// For skinning splits due to shader constants limit only

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling (Skinned, EnableInstancing etc too?)
	//can use Flags field of CNodeAttribute

	virtual bool			LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count);
	virtual IRenderable*	Clone();
	virtual bool			GetLocalAABB(CAABB& OutBox, UPTR LOD) const;
};

}
