#include "RenderPhaseGUI.h"
#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <UI/UIContext.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/SamplerDesc.h>
#include <Render/Sampler.h>
#include <Render/Effect.h>
#include <Data/Params.h>
#include <Core/Factory.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <CEGUI/RenderTarget.h>
#include <CEGUI/System.h>
#include <CEGUI/GUIContext.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseGUI, 'PHUI', Frame::CRenderPhase);

CRenderPhaseGUI::~CRenderPhaseGUI() = default;

bool CRenderPhaseGUI::Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	RenderTargetID = Desc.Get(CStrID("RenderTarget")).GetValue<CStrID>();

	CString ModeStr;
	if (Desc.TryGet<CString>(ModeStr, CStrID("Mode")))
	{
		if (!n_stricmp(ModeStr.CStr(), "opaque")) DrawMode = UI::DrawMode_Opaque;
		else if (!n_stricmp(ModeStr.CStr(), "transparent")) DrawMode = UI::DrawMode_Transparent;
		else DrawMode = UI::DrawMode_All;
	}
	else DrawMode = UI::DrawMode_All;

	const auto EffectID = CStrID(Desc.Get<CString>(CStrID("Effect"), CString::Empty));
	Render::PEffect Effect = GfxMgr.GetEffect(CStrID(EffectID));

	// TODO: move to effect declaration as a default value
	Render::CSamplerDesc SampDesc;
	SampDesc.SetDefaults();
	SampDesc.AddressU = Render::TexAddr_Clamp;
	SampDesc.AddressV = Render::TexAddr_Clamp;
	SampDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	Render::PSampler LinearSampler = GfxMgr.GetGPU()->CreateSampler(SampDesc);

	ShaderWrapperTextured.reset(new CEGUI::CDEMShaderWrapper(GfxMgr.GetGPU(), *Effect, LinearSampler));

	//!!!TODO: non-textured shaders!
	//ShaderWrapperColoured = new CDEMShaderWrapper(*ShaderColoured, this);

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGUI::Render(CView& View)
{
	auto pUICtx = View.GetUIContext();
	if (!pUICtx) FAIL;

	auto pCtx = pUICtx->GetCEGUIContext();
	if (!pCtx) FAIL;

	CEGUI::Renderer* pRenderer = CEGUI::System::getSingleton().getRenderer();
	if (!pRenderer) FAIL;

	auto RT = View.GetRenderTarget(RenderTargetID);
	if (!RT) FAIL;

	View.GetGPU()->SetRenderTarget(0, RT);

	const Render::CRenderTargetDesc& RTDesc = RT->GetDesc();
	//!!!relative(normalized) VP coords may be defined in a phase desc/instance(this) and translated into absolute values here!
	//!!!CEGUI 0.8.4 incorrectly processes viewports with offset!
	float RTWidth = (float)RTDesc.Width;
	float RTHeight = (float)RTDesc.Height;
	float ViewportRelLeft = 0.f;
	float ViewportRelTop = 0.f;
	float ViewportRelRight = 1.f;
	float ViewportRelBottom = 1.f;

	if (!DrawMode) OK;

	CEGUI::Rectf ViewportArea(ViewportRelLeft * RTWidth, ViewportRelTop * RTHeight, ViewportRelRight * RTWidth, ViewportRelBottom * RTHeight);
	if (pCtx->getRenderTarget().getArea() != ViewportArea)
		pCtx->getRenderTarget().setArea(ViewportArea);

	// FIXME: CEGUI drawMode concept doesn't fully fit into requirements of
	// opaque/transparent separation, so draw all in a transparent phase for now.
	if (!(DrawMode & UI::DrawMode_Transparent)) OK;

	static_cast<CEGUI::CDEMRenderer*>(pRenderer)->setEffects(ShaderWrapperTextured.get(), ShaderWrapperColored.get());
	pRenderer->beginRendering();

	//if (Mode & DrawMode_Opaque)
	//	pCtx->draw(DrawModeFlagWindowOpaque);

	//if (Mode & DrawMode_Transparent)
	//	pCtx->draw(CEGUI::DrawModeFlagWindowRegular | CEGUI::DrawModeFlagMouseCursor);
	pCtx->draw();

	pRenderer->endRendering();

	OK;
}
//---------------------------------------------------------------------

}
