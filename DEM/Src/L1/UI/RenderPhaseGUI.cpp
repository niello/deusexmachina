#include "RenderPhaseGUI.h"

#include <Frame/View.h>
#include <UI/UIContext.h>
#include <Render/GPUDriver.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CRenderPhaseGUI, 'PHUI', Frame::CRenderPhase);

bool CRenderPhaseGUI::Render(CView& View)
{
	if (View.UIContext.IsNullPtr()) FAIL;
	View.GPU->SetRenderTarget(0, View.RTs[0]);
	//!!!???here CEGUI must init viewport size?!
	return View.UIContext->Render();
}
//---------------------------------------------------------------------

}