#include "RenderPhaseGeometry.h"
#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/CameraAttribute.h>
#include <Render/Renderable.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseGeometry, 'PHGE', Frame::CRenderPhase);

bool CRenderPhaseGeometry::Render(CView& View)
{
	auto pGPU = View.GetGPU();

	ZoneScoped;
	ZoneText(Name.CStr(), std::strlen(Name.CStr()));
	DEM_RENDER_EVENT_SCOPED(pGPU, std::wstring(Name.CStr(), Name.CStr() + std::strlen(Name.CStr())).c_str());

	if (!View.GetGraphicsScene() || !View.GetCamera()) OK;

	// Bind render targets and a depth-stencil buffer

	const UPTR RenderTargetCount = _RenderTargetIDs.size();
	for (UPTR i = 0; i < RenderTargetCount; ++i)
	{
		auto pTarget = View.GetRenderTarget(_RenderTargetIDs[i]);
		pGPU->SetRenderTarget(i, pTarget);
		pGPU->SetViewport(i, &Render::GetRenderTargetViewport(pTarget->GetDesc()));
	}

	const UPTR MaxRTCount = pGPU->GetMaxMultipleRenderTargetCount();
	for (UPTR i = RenderTargetCount; i < MaxRTCount; ++i)
		pGPU->SetRenderTarget(i, nullptr);

	auto pDepthStencliBuffer = View.GetDepthStencilBuffer(_DepthStencilID);
	if (pDepthStencliBuffer && !RenderTargetCount)
		pGPU->SetViewport(0, &Render::GetRenderTargetViewport(pDepthStencliBuffer->GetDesc()));
	pGPU->SetDepthStencilBuffer(pDepthStencliBuffer);

	// Render objects from queues

	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.pShaderTechCache = View.GetShaderTechCache(_ShaderTechCacheIndex);

	Render::IRenderer* pCurrRenderer = nullptr;
	U8 CurrRendererIndex = 0;
	for (const U32 QueueIndex : _RenderQueueIndices)
	{
		View.ForEachRenderableInQueue(QueueIndex, [this, &View, &Ctx, &pCurrRenderer, &CurrRendererIndex](Render::IRenderable* pRenderable)
		{
			// This is guaranteed by CView, it fills queues with visible objects only
			n_assert_dbg(pRenderable->IsVisible);

			if (CurrRendererIndex != pRenderable->RendererIndex)
			{
				if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);
				CurrRendererIndex = pRenderable->RendererIndex;
				pCurrRenderer = View.GetRenderer(CurrRendererIndex);
				if (pCurrRenderer)
					if (!pCurrRenderer->BeginRange(Ctx))
						pCurrRenderer = nullptr;
			}

			if (pCurrRenderer) pCurrRenderer->Render(Ctx, *pRenderable);
		});
	}
	if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);

	// Unbind render target(s) etc
	//???allow each phase to declare all its RTs and clear unused ones by itself?
	//then unbind in the end of a CRenderPath::Render()

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Init(CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	const Data::CData& RTValue = Desc.Get(CStrID("RenderTarget")).GetRawValue();
	if (RTValue.IsNull()) _RenderTargetIDs.SetSize(0);
	else if (RTValue.IsA<Data::PDataArray>())
	{
		Data::PDataArray RTArray = RTValue.GetValue<Data::PDataArray>();
		_RenderTargetIDs.SetSize(RTArray->size());
		for (UPTR i = 0; i < RTArray->size(); ++i)
		{
			const Data::CData& RTElm = RTArray->Get<int>(i);
			if (RTElm.IsNull()) _RenderTargetIDs[i] = CStrID::Empty;
			else if (RTElm.IsA<int>()) _RenderTargetIDs[i] = RTArray->Get<CStrID>(i);
			else FAIL;
		}
	}
	else if (RTValue.IsA<CStrID>())
	{
		_RenderTargetIDs.SetSize(1);
		_RenderTargetIDs[0] = RTValue.GetValue<CStrID>();
	}
	else FAIL;

	const Data::CData& DSValue = Desc.Get(CStrID("DepthStencilBuffer")).GetRawValue();
	if (DSValue.IsNull()) _DepthStencilID = CStrID::Empty;
	else if (DSValue.IsA<CStrID>())
	{
		_DepthStencilID = DSValue.GetValue<CStrID>();
	}
	else FAIL;

	const auto& RenderQueuesDesc = *Desc.Get<Data::PDataArray>(CStrID("RenderQueues"));
	_RenderQueueIndices.resize(RenderQueuesDesc.size());
	for (UPTR i = 0; i < RenderQueuesDesc.size(); ++i)
	{
		const CStrID RenderQueueType = RenderQueuesDesc.Get<CStrID>(i);
		auto It = Owner._RenderQueues.find(RenderQueueType);
		if (It == Owner._RenderQueues.cend())
			It = Owner._RenderQueues.emplace(RenderQueueType, Owner._RenderQueues.size()).first;
		_RenderQueueIndices[i] = It->second;
	}

	Data::PParams EffectsDesc;
	if (Desc.TryGet(EffectsDesc, CStrID("Effects")))
	{
		std::map<Render::EEffectType, CStrID> EffectOverrides;
		for (UPTR i = 0; i < EffectsDesc->GetCount(); ++i)
		{
			const Data::CParam& Prm = EffectsDesc->Get(i);

			Render::EEffectType EffectType;
			CStrID Key = Prm.GetName();
			if (Key == "Opaque") EffectType = Render::EffectType_Opaque;
			else if (Key == "AlphaTest") EffectType = Render::EffectType_AlphaTest;
			else if (Key == "Skybox") EffectType = Render::EffectType_Skybox;
			else if (Key == "AlphaBlend") EffectType = Render::EffectType_AlphaBlend;
			else if (Key == "Other") EffectType = Render::EffectType_Other;
			else FAIL;

			EffectOverrides.emplace(EffectType, Prm.GetRawValue().IsNull() ? CStrID::Empty : Prm.GetValue<CStrID>());
		}

		// _ShaderTechCacheIndex 0 is used for original effects, 1 and above are for overrides
		if (!EffectOverrides.empty())
		{
			Owner.EffectOverrides.push_back(std::move(EffectOverrides));
			_ShaderTechCacheIndex = Owner.EffectOverrides.size();
		}
		else
		{
			_ShaderTechCacheIndex = 0;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}
