#pragma once
#include <Frame/RenderableAttribute.h>

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

	Render::PMeshData _MeshData;

public:

	CModelAttribute() = default;
	CModelAttribute(CStrID MeshUID, CStrID MaterialUID, U32 MeshGroupIndex = 0);

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual bool                  ValidateResources(Resources::CResourceManager& ResMgr) override;
	virtual Render::PRenderable   CreateRenderable() const override;
	virtual void                  UpdateRenderable(CView& View, Render::IRenderable& Renderable, bool ViewProjChanged) const override;
	virtual void                  UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const override;
	virtual void                  OnLightIntersectionsUpdated() override {}
	virtual U8                    GetLightTrackingFlags() const override { return TrackLightContactChanges; }
	virtual bool                  GetLocalAABB(Math::CAABB& OutBox, UPTR LOD = 0) const override;
};

typedef Ptr<CModelAttribute> PModelAttribute;

}
