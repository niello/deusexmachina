#pragma once
#include <Frame/RenderableAttribute.h>
#include <Data/FixedArray.h>

// Scene node attribute with renderable model

namespace Render
{
	typedef Ptr<class CMeshData> PMeshData;
}

namespace Frame
{

class CModelAttribute : public CRenderableAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID            _MeshUID;
	CStrID            _MaterialUID;
	U32               _MeshGroupIndex = 0;
	CFixedArray<int>  _BoneIndices;        // For skinning splits due to shader constants limit

	Render::PMeshData _MeshData;

public:

	CModelAttribute() = default;
	CModelAttribute(CStrID MeshUID, CStrID MaterialUID, U32 MeshGroupIndex = 0);

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual bool                  ValidateGPUResources(CGraphicsResourceManager& ResMgr) override;
	virtual bool                  GetLocalAABB(CAABB& OutBox, UPTR LOD = 0) const override;
};

typedef Ptr<CModelAttribute> PModelAttribute;

}
