#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/GPUDriver.h>
#include <Render/ShaderMetadata.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CRenderPath, Resources::CResourceObject);

CRenderPath::~CRenderPath()
{
	if (pGlobals) n_delete(pGlobals);
}
//---------------------------------------------------------------------

bool CRenderPath::Render(CView& View)
{
	// We clear all phases' render targets and DS surfaces at the beginning of
	// the frame, as recommended, especially for SLI. It also serves for a better
	// rendering architecture, as phase must not clear RTs and therefore know
	// whether it is the first who writes to the target.

	for (UPTR i = 0; i < RTSlots.GetCount(); ++i)
	{
		Render::CRenderTarget* pRT = View.RTs[i].GetUnsafe();
		if (pRT) View.GPU->ClearRenderTarget(*pRT, RTSlots[i].ClearValue);
	}

	for (UPTR i = 0; i < DSSlots.GetCount(); ++i)
	{
		Render::CDepthStencilBuffer* pDS = View.DSBuffers[i].GetUnsafe();
		if (pDS)
		{
			const CDepthStencilSlot& Slot = DSSlots[i];
			View.GPU->ClearDepthStencilBuffer(*pDS, Slot.ClearFlags, Slot.DepthClearValue, Slot.StencilClearValue);
		}
	}

	for (UPTR i = 0; i < Phases.GetCount(); ++i)
	{
		if (!Phases[i]->Render(View))
		{
			//???clear tmp view data?
			FAIL;
		}
	}

	//???clear tmp view data?

	OK;
}
//---------------------------------------------------------------------

}