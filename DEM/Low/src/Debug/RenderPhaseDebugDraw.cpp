#include "RenderPhaseDebugDraw.h"
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseDebugDraw, 'PHDD', Frame::CRenderPhase);

bool CRenderPhaseDebugDraw::Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	//RenderTargetID = Desc.Get(CStrID("RenderTarget")).GetValue<CStrID>();

	//const auto EffectID = CStrID(Desc.Get<CString>(CStrID("Effect"), CString::Empty));
	//Render::PEffect Effect = GfxMgr.GetEffect(CStrID(EffectID));

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseDebugDraw::Render(CView& View)
{
	OK;
}
//---------------------------------------------------------------------

}