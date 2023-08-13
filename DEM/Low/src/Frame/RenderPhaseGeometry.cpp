#include "RenderPhaseGeometry.h"
#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/CameraAttribute.h>
#include <Render/Renderable.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/ImageBasedLight.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseGeometry, 'PHGE', Frame::CRenderPhase);

CRenderPhaseGeometry::CRenderPhaseGeometry() = default;
//---------------------------------------------------------------------

CRenderPhaseGeometry::~CRenderPhaseGeometry() = default;
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Render(CView& View)
{
	if (!View.GetGraphicsScene() || !View.GetCamera()) OK;

	if (!View.GetGraphicsManager()) FAIL;

	n_assert_dbg(!View.LightIndices.GetCount());

	// Setup global lighting params, both ambient and direct

	auto pGPU = View.GetGPU();

	// TODO: move to the global part of rendering! Do once for all phases!
	if (EnableLighting)
	{
		if (ConstGlobalLightBuffer)
		{
			//!!!for a structured buffer, max count may be not applicable! must then use the same value
			//as was used to allocate structured buffer instance!
			// e.g. see https://www.gamedev.net/forums/topic/709796-working-with-structuredbuffer-in-hlsl-directx-11/
			// StructuredBuffer<Light> lights : register(t9);
			UPTR GlobalLightCount = 0;
			const UPTR MaxLightCount = ConstGlobalLightBuffer.GetElementCount();
			n_assert_dbg(MaxLightCount > 0);

			/*
			const CArray<Render::CLightRecord>& VisibleLights = View.GetLightCache();
			for (size_t i = 0; i < VisibleLights.GetCount(); ++i)
			{
				Render::CLightRecord& LightRec = VisibleLights[i];
				if (LightRec.UseCount)
				{
					const Render::CLight_OLD_DELETE& Light = *LightRec.pLight;

					struct
					{
						vector3	Color;
						float	_PAD1;
						vector3	Position;
						float	SqInvRange;		// For attenuation
						vector4	Params;			// Spot: x - cos inner, y - cos outer
						vector3	InvDirection;
						U32		Type;
					} GPULight;

					GPULight.Color = Light.Color * Light.Intensity; //???pre-multiply and don't store separately at all?
					GPULight.Position = LightRec.Transform.Translation();
					GPULight.SqInvRange = Light.GetInvRange() * Light.GetInvRange();
					GPULight.InvDirection = LightRec.Transform.AxisZ();
					if (Light.Type == Render::Light_Spot)
					{
						GPULight.Params.x = Light.GetCosHalfTheta();
						GPULight.Params.y = Light.GetCosHalfPhi();
					}
					GPULight.Type = Light.Type;

					View.Globals.SetRawConstant(ConstGlobalLightBuffer[GlobalLightCount], &GPULight, sizeof(GPULight));

					LightRec.GPULightIndex = GlobalLightCount;
					++GlobalLightCount;
					if (GlobalLightCount >= MaxLightCount) break;
				}
			}
			*/

			if (GlobalLightCount) View.Globals.Apply();
		}

		// Setup IBL (ambient cubemaps)
		// TODO: later can implement local weight-blended parallax-corrected cubemaps selected by COI
		//https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/

		//???need visibility check for env maps? or select through separate spatial query?! SPS.FindClosest(COI, AttrRTTI, MaxCount)!
		//may refresh only when COI changes, because maps are considered static
		auto& EnvCache = View.GetEnvironmentCache();
		if (EnvCache.GetCount())
		{
			auto* pGlobalAmbientLight = EnvCache[0];

			if (RsrcIrradianceMap)
				RsrcIrradianceMap->Apply(*pGPU, pGlobalAmbientLight->_IrradianceMap);

			if (RsrcRadianceEnvMap)
				RsrcRadianceEnvMap->Apply(*pGPU, pGlobalAmbientLight->_RadianceEnvMap);

			if (SampTrilinearCube)
				SampTrilinearCube->Apply(*pGPU, View.TrilinearCubeSampler);
		}
	}

	// Bind render targets and a depth-stencil buffer

	const UPTR RenderTargetCount = RenderTargetIDs.size();
	for (UPTR i = 0; i < RenderTargetCount; ++i)
	{
		auto pTarget = View.GetRenderTarget(RenderTargetIDs[i]);
		pGPU->SetRenderTarget(i, pTarget);
		pGPU->SetViewport(i, &Render::GetRenderTargetViewport(pTarget->GetDesc()));
	}

	const UPTR MaxRTCount = pGPU->GetMaxMultipleRenderTargetCount();
	for (UPTR i = RenderTargetCount; i < MaxRTCount; ++i)
		pGPU->SetRenderTarget(i, nullptr);

	auto pDepthStencliBuffer = View.GetDepthStencilBuffer(DepthStencilID);
	if (pDepthStencliBuffer && !RenderTargetCount)
		pGPU->SetViewport(0, &Render::GetRenderTargetViewport(pDepthStencliBuffer->GetDesc()));
	pGPU->SetDepthStencilBuffer(pDepthStencliBuffer);

	// Render objects from queues

	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.pShaderTechCache = View.GetShaderTechCache(_ShaderTechCacheIndex);
	Ctx.CameraPosition = View.GetCamera()->GetPosition();
	Ctx.ViewProjection = View.GetCamera()->GetViewProjMatrix();
	if (EnableLighting)
	{
		//Ctx.pLights = &View.GetLightCache();
		//Ctx.pLightIndices = &View.LightIndices;
		//Ctx.UsesGlobalLightBuffer = !!ConstGlobalLightBuffer;
	}

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

	//!!!hide in a private method of CView!
	View.LightIndices.Clear(false);

	// Unbind render target(s) etc
	//???allow each phase to declare all its RTs and clear unused ones by itself?
	//then unbind in the end of a CRenderPath::Render()

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Init(CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	EnableLighting = Desc.Get<bool>(CStrID("EnableLighting"), false);

	const Data::CData& RTValue = Desc.Get(CStrID("RenderTarget")).GetRawValue();
	if (RTValue.IsNull()) RenderTargetIDs.SetSize(0);
	else if (RTValue.IsA<Data::PDataArray>())
	{
		Data::PDataArray RTArray = RTValue.GetValue<Data::PDataArray>();
		RenderTargetIDs.SetSize(RTArray->GetCount());
		for (UPTR i = 0; i < RTArray->GetCount(); ++i)
		{
			const Data::CData& RTElm = RTArray->Get<int>(i);
			if (RTElm.IsNull()) RenderTargetIDs[i] = CStrID::Empty;
			else if (RTElm.IsA<int>()) RenderTargetIDs[i] = RTArray->Get<CStrID>(i);
			else FAIL;
		}
	}
	else if (RTValue.IsA<CStrID>())
	{
		RenderTargetIDs.SetSize(1);
		RenderTargetIDs[0] = RTValue.GetValue<CStrID>();
	}
	else FAIL;

	const Data::CData& DSValue = Desc.Get(CStrID("DepthStencilBuffer")).GetRawValue();
	if (DSValue.IsNull()) DepthStencilID = CStrID::Empty;
	else if (DSValue.IsA<CStrID>())
	{
		DepthStencilID = DSValue.GetValue<CStrID>();
	}
	else FAIL;

	const auto& RenderQueuesDesc = *Desc.Get<Data::PDataArray>(CStrID("RenderQueues"));
	_RenderQueueIndices.resize(RenderQueuesDesc.GetCount());
	for (UPTR i = 0; i < RenderQueuesDesc.GetCount(); ++i)
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

	// Cache global shader parameter descriptions

	const auto& GlobalParams = Owner.GetGlobalParamTable();

	CStrID GlobalLightBufferName = Desc.Get<CStrID>(CStrID("GlobalLightBufferName"), CStrID::Empty);
	if (GlobalLightBufferName.IsValid())
		ConstGlobalLightBuffer = GlobalParams.GetConstant(GlobalLightBufferName);

	CStrID IrradianceMapName = Desc.Get<CStrID>(CStrID("IrradianceMapName"), CStrID::Empty);
	if (IrradianceMapName.IsValid())
		RsrcIrradianceMap = GlobalParams.GetResource(IrradianceMapName);

	CStrID RadianceEnvMapName = Desc.Get<CStrID>(CStrID("RadianceEnvMapName"), CStrID::Empty);
	if (RadianceEnvMapName.IsValid())
		RsrcRadianceEnvMap = GlobalParams.GetResource(RadianceEnvMapName);

	CStrID TrilinearCubeSamplerName = Desc.Get<CStrID>(CStrID("TrilinearCubeSamplerName"), CStrID::Empty);
	if (TrilinearCubeSamplerName.IsValid())
		SampTrilinearCube = GlobalParams.GetSampler(TrilinearCubeSamplerName);

	OK;
}
//---------------------------------------------------------------------

}
