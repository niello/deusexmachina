#pragma once
#include <Render/Renderable.h>
#include <Data/Ptr.h>
#include <Data/FixedArray.h>
#include <Data/StringID.h>

// Model represents a piece of visible polygonal geometry.
// It ties together a mesh group, a material and per-object rendering params.

namespace Render
{
typedef Ptr<class CMesh> PMesh;
typedef Ptr<class CMeshData> PMeshData;
typedef Ptr<class CMaterial> PMaterial;

class CModel: public IRenderable
{
	FACTORY_CLASS_DECL;

public:

	PMaterial        Material; //???!!!materialset!?
	PMesh            Mesh;
	PMeshData        MeshData;           // For GPU-independent AABB access
	U32              MeshGroupIndex = 0;
	CFixedArray<int> BoneIndices;        // For skinning splits due to shader constants limit only

	// ERenderFlag: ShadowCaster, ShadowReceiver, DoOcclusionCulling (Skinned, EnableInstancing etc too?)

	virtual PRenderable Clone() override;
	virtual bool        GetLocalAABB(CAABB& OutBox, UPTR LOD) const override;
};

}
