#pragma once
#include <Frame/RenderableAttribute.h>

// Scene node attribute with renderable skybox

namespace Frame
{

class CSkyboxAttribute: public CRenderableAttribute
{
	FACTORY_CLASS_DECL;

protected:

	CStrID _MaterialUID;

public:

	virtual bool                  LoadDataBlocks(IO::CBinaryReader& DataReader, UPTR Count) override;
	virtual Scene::PNodeAttribute Clone() override;
	virtual Render::PRenderable   CreateRenderable() const override;
	virtual void                  UpdateRenderable(CView& View, Render::IRenderable& Renderable, bool ViewProjChanged) const override;
	virtual void                  UpdateLightList(CView& View, Render::IRenderable& Renderable, const CObjectLightIntersection* pHead) const override { n_assert_dbg(!pHead); }
	virtual void                  OnLightIntersectionsUpdated() override {}
	virtual U8                    GetLightTrackingFlags() const override { return 0; }
	virtual bool                  GetLocalAABB(Math::CAABB& OutBox, UPTR LOD = 0) const override;
};

typedef Ptr<CSkyboxAttribute> PSkyboxAttribute;

}
