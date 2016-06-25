#include "RenderPhaseGUI.h"

#include <Frame/View.h>
#include <UI/UIContext.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CRenderPhaseGUI, 'PHUI', Frame::CRenderPhase);

bool CRenderPhaseGUI::Render(CView& View)
{
	if (View.UIContext.IsNullPtr()) FAIL;
	View.GPU->SetRenderTarget(0, View.RTs[0]);
	const Render::CRenderTargetDesc& RTDesc = View.RTs[0]->GetDesc();
	//!!!relative(normalized) VP coords may be defined in a phase desc/instance(this) and translated into absolute values here!
	//!!!CEGUI 0.8.4 incorrectly processes viewports with offset!
	float RTWidth = (float)RTDesc.Width;
	float RTHeight = (float)RTDesc.Height;
	float ViewportRelLeft = 0.f;
	float ViewportRelTop = 0.f;
	float ViewportRelRight = 1.f;
	float ViewportRelBottom = 1.f;
	return View.UIContext->Render(ViewportRelLeft * RTWidth, ViewportRelTop * RTHeight, ViewportRelRight * RTWidth, ViewportRelBottom * RTHeight);
}
//---------------------------------------------------------------------

bool CRenderPhaseGUI::Init(CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(PhaseName, Desc)) FAIL;

	RenderTargetIndex = (I32)Desc.Get(CStrID("RenderTarget")).GetValue<int>();

	OK;
}
//---------------------------------------------------------------------

}