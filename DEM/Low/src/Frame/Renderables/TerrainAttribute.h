#pragma once
#include <Frame/RenderableAttribute.h>

// Scene node attribute with renderable terrain

namespace Render
{
	typedef Ptr<class CCDLODData> PCDLODData;
}

namespace Frame
{

class CTerrainAttribute: public CRenderableAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID             _MaterialUID;
	CStrID             _CDLODDataUID;
	CStrID             _HeightMapUID;
	float              _InvSplatSizeX = 1.f;
	float              _InvSplatSizeZ = 1.f;

	Render::PCDLODData _CDLODData;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual Render::PRenderable   CreateRenderable() const override;
	virtual void                  UpdateRenderable(CView& View, Render::IRenderable& Renderable) const override;
	virtual void                  UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const override;
	virtual bool                  GetLocalAABB(CAABB& OutBox, UPTR LOD = 0) const override;
};

typedef Ptr<CTerrainAttribute> PTerrainAttribute;

}
