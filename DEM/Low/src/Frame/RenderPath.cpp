#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/GPUDriver.h>
#include <Render/ShaderMetadata.h>
#include <Render/ShaderConstant.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CRenderPath, Resources::CResourceObject);

CRenderPath::CRenderPath() {}

CRenderPath::~CRenderPath()
{
	if (pGlobals) n_delete(pGlobals);
}
//---------------------------------------------------------------------

const Render::CEffectConstant* CRenderPath::GetGlobalConstant(CStrID Name) const
{
	UPTR i = 0;
	for (; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Curr = Consts[i];
		if (Curr.ID == Name) return &Curr;
	}
	return NULL;
}
//---------------------------------------------------------------------

const Render::CEffectResource* CRenderPath::GetGlobalResource(CStrID Name) const
{
	UPTR i = 0;
	for (; i < Resources.GetCount(); ++i)
	{
		const Render::CEffectResource& Curr = Resources[i];
		if (Curr.ID == Name) return &Curr;
	}
	return NULL;
}
//---------------------------------------------------------------------

const Render::CEffectSampler* CRenderPath::GetGlobalSampler(CStrID Name) const
{
	UPTR i = 0;
	for (; i < Samplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Curr = Samplers[i];
		if (Curr.ID == Name) return &Curr;
	}
	return NULL;
}
//---------------------------------------------------------------------

void CRenderPath::SetRenderTargetClearColor(UPTR Index, const vector4& Color)
{
	if (Index >= RTSlots.GetCount()) return;
	RTSlots[Index].ClearValue = Color;
}
//---------------------------------------------------------------------

bool CRenderPath::Render(CView& View)
{
	if (!View.GPU->BeginFrame()) FAIL;

	// We clear all phases' render targets and DS surfaces at the beginning of
	// the frame, as recommended, especially for SLI. It also serves for a better
	// rendering architecture, as phase must not clear RTs and therefore know
	// whether it is the first who writes to the target.

	for (UPTR i = 0; i < RTSlots.GetCount(); ++i)
	{
		Render::CRenderTarget* pRT = View.RTs[i].Get();
		if (pRT) View.GPU->ClearRenderTarget(*pRT, RTSlots[i].ClearValue);
	}

	for (UPTR i = 0; i < DSSlots.GetCount(); ++i)
	{
		Render::CDepthStencilBuffer* pDS = View.DSBuffers[i].Get();
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

	View.GPU->EndFrame();

	OK;
}
//---------------------------------------------------------------------

}