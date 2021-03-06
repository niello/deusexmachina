#include "RenderPhaseDebugDraw.h"
#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/CameraAttribute.h>
#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Debug/DebugDraw.h>
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseDebugDraw, 'PHDD', Frame::CRenderPhase);

CRenderPhaseDebugDraw::~CRenderPhaseDebugDraw() = default;

bool CRenderPhaseDebugDraw::Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	RenderTargetID = Desc.Get(CStrID("RenderTarget")).GetValue<CStrID>();

	const auto EffectID = CStrID(Desc.Get<CString>(CStrID("Effect"), CString::Empty));
	Effect = GfxMgr.GetEffect(CStrID(EffectID));

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseDebugDraw::Render(CView& View)
{
	const auto pCamera = View.GetCamera();
	const auto pDebugDraw = View.GetDebugDrawer();
	auto RT = View.GetRenderTarget(RenderTargetID);
	if (!pCamera || !pDebugDraw || !Effect || !RT) FAIL;

	View.GetGPU()->SetRenderTarget(0, RT);

	pDebugDraw->Render(*Effect, pCamera->GetViewProjMatrix());

	OK;
}
//---------------------------------------------------------------------

}