#include "RenderPhaseDebugDraw.h"
#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/CameraAttribute.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/Effect.h>
#include <Debug/DebugDraw.h>
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseDebugDraw, 'PHDD', Frame::CRenderPhase);

CRenderPhaseDebugDraw::~CRenderPhaseDebugDraw() = default;

bool CRenderPhaseDebugDraw::Init(CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	RenderTargetID = Desc.Get(CStrID("RenderTarget")).GetValue<CStrID>();

	const auto EffectID = CStrID(Desc.Get<std::string>(CStrID("Effect"), EmptyString));
	Effect = GfxMgr.GetEffect(CStrID(EffectID));

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseDebugDraw::Render(CView& View)
{
	ZoneScoped;
	ZoneText(Name.CStr(), std::strlen(Name.CStr()));
	DEM_RENDER_EVENT_SCOPED(View.GetGPU(), std::wstring(Name.CStr(), Name.CStr() + std::strlen(Name.CStr())).c_str());

	const auto pCamera = View.GetCamera();
	const auto pDebugDraw = View.GetDebugDrawer();
	auto pTarget = View.GetRenderTarget(RenderTargetID);
	if (!pCamera || !pDebugDraw || !Effect || !pTarget) FAIL;

	View.GetGPU()->SetRenderTarget(0, pTarget);
	View.GetGPU()->SetViewport(0, &Render::GetRenderTargetViewport(pTarget->GetDesc()));

	pDebugDraw->Render(*Effect, pCamera->GetViewProjMatrix());

	OK;
}
//---------------------------------------------------------------------

}
