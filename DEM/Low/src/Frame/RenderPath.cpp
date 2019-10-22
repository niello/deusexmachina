#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/GPUDriver.h>
#include <Render/ShaderMetadata.h>
#include <Render/ShaderConstant.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CRenderPath, Core::CObject);

CRenderPath::CRenderPath() {}

CRenderPath::~CRenderPath() {}

void CRenderPath::SetRenderTargetClearColor(CStrID ID, const vector4& Color)
{
	auto It = RTSlots.find(ID);
	if (It != RTSlots.cend()) It->second.ClearValue = Color;
}
//---------------------------------------------------------------------

bool CRenderPath::Render(CView& View)
{
	Render::CGPUDriver* pGPU = View.GetGPU();
	if (!pGPU || !pGPU->BeginFrame()) FAIL;

	// We clear all phases' render targets and DS surfaces at the beginning of
	// the frame, as recommended, especially for SLI. It also serves for a better
	// rendering architecture, as phase must not clear RTs and therefore know
	// whether it is the first who writes to the target.

	for (const auto& Slot : RTSlots)
	{
		Render::CRenderTarget* pRT = View.GetRenderTarget(Slot.first);
		if (pRT) pGPU->ClearRenderTarget(*pRT, Slot.second.ClearValue);
	}

	for (const auto& Slot : DSSlots)
	{
		Render::CDepthStencilBuffer* pDS = View.GetDepthStencilBuffer(Slot.first);
		if (pDS)
			pGPU->ClearDepthStencilBuffer(*pDS, Slot.second.ClearFlags, Slot.second.DepthClearValue, Slot.second.StencilClearValue);
	}

	for (auto& Phase : Phases)
	{
		if (!Phase->Render(View))
		{
			//???clear tmp view data?
			FAIL;
		}
	}

	//???clear tmp view data?

	pGPU->EndFrame();

	OK;
}
//---------------------------------------------------------------------

}