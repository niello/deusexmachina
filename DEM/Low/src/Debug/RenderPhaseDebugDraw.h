#pragma once
#include <Frame/RenderPhase.h>

// Frame rendering phase that draws debug elements

namespace Frame
{

class CRenderPhaseDebugDraw: public CRenderPhase
{
	FACTORY_CLASS_DECL;

private:

	CStrID RenderTargetID;

public:

	virtual bool Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) override;
	virtual bool Render(CView& View) override;
};

}
