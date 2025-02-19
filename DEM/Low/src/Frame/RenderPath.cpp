#include "RenderPath.h"
#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/CameraAttribute.h>
#include <Render/GPUDriver.h>
#include <Data/Params.h> // For the PParams destructor

namespace Frame
{
CRenderPath::CRenderPath() = default;
CRenderPath::~CRenderPath() = default;

void CRenderPath::AddRenderTargetSlot(CStrID ID, vector4 ClearValue)
{
	RTSlots.emplace(ID, CRenderTargetSlot{ ClearValue });
}
//---------------------------------------------------------------------

void CRenderPath::AddDepthStencilSlot(CStrID ID, U32 ClearFlags, float DepthClearValue, U8 StencilClearValue)
{
	DSSlots.emplace(ID, CDepthStencilSlot{ ClearFlags, DepthClearValue, StencilClearValue });
}
//---------------------------------------------------------------------

void CRenderPath::SetRenderTargetClearColor(CStrID ID, const vector4& Color)
{
	auto It = RTSlots.find(ID);
	if (It != RTSlots.cend()) It->second.ClearValue = Color;
}
//---------------------------------------------------------------------

bool CRenderPath::Render(CView& View)
{
	ZoneScoped;

	Render::CGPUDriver* pGPU = View.GetGPU();
	if (!pGPU || !pGPU->BeginFrame()) FAIL;

	// We clear all phases' render targets and DS surfaces at the beginning of
	// the frame, as recommended, especially for SLI. It also serves for a better
	// rendering architecture, as phase must not clear RTs and therefore know
	// whether it is the first who writes to the target.

	{
		ZoneScopedN("clear targets");

		for (const auto& Slot : RTSlots)
			if (auto pRT = View.GetRenderTarget(Slot.first))
				pGPU->ClearRenderTarget(*pRT, Slot.second.ClearValue);

		for (const auto& Slot : DSSlots)
			if (auto pDS = View.GetDepthStencilBuffer(Slot.first))
				pGPU->ClearDepthStencilBuffer(*pDS, Slot.second.ClearFlags, Slot.second.DepthClearValue, Slot.second.StencilClearValue);
	}

	View.ApplyGlobalShaderParams();

	// Do rendering

	bool AllPhasesSucceeded = true;
	for (auto& Phase : Phases)
		AllPhasesSucceeded |= Phase->Render(View);

	//???clear tmp view data?
	pGPU->EndFrame();
	return AllPhasesSucceeded;
}
//---------------------------------------------------------------------

}
