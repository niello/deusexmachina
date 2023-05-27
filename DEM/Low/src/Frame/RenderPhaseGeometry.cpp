#include "RenderPhaseGeometry.h"

#include <Frame/View.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/RenderPath.h>
#include <Frame/CameraAttribute.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/Lights/IBLAmbientLightAttribute.h>
#include <Frame/SkinAttribute.h>
#include <Frame/SkinProcessorAttribute.h>
#include <Render/Renderable.h>
#include <Render/Renderer.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/SkinInfo.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/ImageBasedLight.h>
#include <Scene/SceneNode.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Resources/ResourceCreator.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Frame
{
FACTORY_CLASS_IMPL(Frame::CRenderPhaseGeometry, 'PHGE', Frame::CRenderPhase);

constexpr UPTR AlphaBlendStartOrder = 40;

struct CRenderQueueCmp_FrontToBack
{
	inline bool operator()(const Render::CRenderNode* a, const Render::CRenderNode* b) const
	{
		if (a->Order != b->Order) return a->Order < b->Order;
		return a->SqDistanceToCamera < b->SqDistanceToCamera;
	}
};
//---------------------------------------------------------------------

struct CRenderQueueCmp_Material
{
	inline bool operator()(const Render::CRenderNode* a, const Render::CRenderNode* b) const
	{
		if (a->Order != b->Order) return a->Order < b->Order;
		if (a->Order >= AlphaBlendStartOrder)
		{
			return a->SqDistanceToCamera > b->SqDistanceToCamera;
		}
		else
		{
			if (a->pTech != b->pTech) return a->pTech < b->pTech;
			if (a->pMaterial != b->pMaterial) return a->pMaterial < b->pMaterial;
			if (a->pMesh != b->pMesh) return a->pMesh < b->pMesh;
			if (a->pGroup != b->pGroup) return a->pGroup < b->pGroup;
			return a->SqDistanceToCamera < b->SqDistanceToCamera;
		}
	}
};
//---------------------------------------------------------------------

CRenderPhaseGeometry::CRenderPhaseGeometry() = default;
//---------------------------------------------------------------------

CRenderPhaseGeometry::~CRenderPhaseGeometry() = default;
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Render(CView& View)
{
	if (!View.GetGraphicsScene() || !View.GetCamera()) OK;

	if (!View.GetGraphicsManager()) FAIL;

	auto& VisibleObjects = View.GetVisibilityCache();

	if (!VisibleObjects.GetCount()) OK;

	const vector3& CameraPos = View.GetCamera()->GetPosition();
	const bool CalcScreenSize = View.RequiresObjectScreenSize();

	auto& RenderQueue = View.RenderQueue;
	RenderQueue.Resize(VisibleObjects.GetCount());

	n_assert_dbg(!View.LightIndices.GetCount());

	Render::CRenderNodeContext Context;

	// Find override effects for the current GPU
	Context.EffectOverrides.clear();
	for (const auto& [Key, EffectUID] : EffectOverrides)
		Context.EffectOverrides.emplace(Key, View.GetGraphicsManager()->GetEffect(EffectUID));

	if (EnableLighting)
	{
		Context.pLights = &View.GetLightCache();
		Context.pLightIndices = &View.LightIndices;
	}
	else
	{
		Context.pLights = nullptr;
		Context.pLightIndices = nullptr;
	}

	for (auto UID : VisibleObjects)
	{
		//!!!DBG TMP!
		auto pAttr = static_cast<Frame::CRenderableAttribute*>(View.GetGraphicsScene()->GetRenderables().find(UID)->second.pAttr);
		Render::IRenderable* pRenderable = View.GetRenderable(UID);

		auto ItRenderer = RenderersByObjectType.find(pRenderable->GetRTTI());
		if (ItRenderer == RenderersByObjectType.cend()) continue;

		Render::CRenderNode* pNode = View.RenderNodePool.Construct();
		pNode->pRenderable = pRenderable;
		pNode->pRenderer = ItRenderer->second;
		pNode->Transform = pAttr->GetNode()->GetWorldMatrix();

		if (auto pSkinAttr = pAttr->GetNode()->FindFirstAttribute<Frame::CSkinAttribute>())
		{
			if (const auto& Palette = pSkinAttr->GetSkinPalette())
			{
				pNode->pSkinPalette = Palette->GetSkinPalette();
				pNode->BoneCount = Palette->GetSkinInfo()->GetBoneCount();
			}
		}
		else pNode->pSkinPalette = nullptr;

		if (pAttr->GetGlobalAABB(Context.AABB, 0))
		{
			float ScreenSizeRel;
			if (CalcScreenSize)
			{
				NOT_IMPLEMENTED_MSG("OBJECT SCREEN SIZE CALCULATION!");
				ScreenSizeRel = 0.f;
			}
			else ScreenSizeRel = 0.f;

			float SqDistanceToCamera = Context.AABB.SqDistance(CameraPos);
			pNode->SqDistanceToCamera = SqDistanceToCamera;
			Context.MeshLOD = View.GetMeshLOD(SqDistanceToCamera, ScreenSizeRel);
			Context.MaterialLOD = View.GetMaterialLOD(SqDistanceToCamera, ScreenSizeRel);
		}
		else
		{
			pNode->SqDistanceToCamera = 0.f;
			Context.MeshLOD = 0;
			Context.MaterialLOD = 0;
		}

		//!!!PERF: needs testing!
		//!!!may send lights subset selected by:
		//if (pAttrRenderable->pSPSRecord->pSPSNode->SharesSpaceWith(*pAttrLight->pSPSRecord->pSPSNode))
		//if (pAttrRenderable->CheckPotentialIntersection(*pAttrLight / pSPSNode)) // true if some of pSPSNodes is nullptr

		if (!pNode->pRenderer->PrepareNode(*pNode, Context))
		{
			View.RenderNodePool.Destroy(pNode);
			continue;
		}

		RenderQueue.Add(pNode);

		n_assert_dbg(pNode->pMaterial);
		Render::EEffectType Type = pNode->pMaterial->GetEffect()->GetType();
		switch (Type)
		{
			case Render::EffectType_Opaque:		pNode->Order = 10; break;
			case Render::EffectType_AlphaTest:	pNode->Order = 20; break;
			case Render::EffectType_Skybox:		pNode->Order = 30; break;
			case Render::EffectType_AlphaBlend:	pNode->Order = AlphaBlendStartOrder; break;
			default:							pNode->Order = 100; break;
		}
	}

	// Setup global lighting params, both ambient and direct

	auto pGPU = View.GetGPU();

	if (EnableLighting)
	{
		if (ConstGlobalLightBuffer)
		{
			//!!!for a structured buffer, max count may be not applicable! must then use the same value
			//as was used to allocate structured buffer instance!
			UPTR GlobalLightCount = 0;
			const UPTR MaxLightCount = ConstGlobalLightBuffer.GetElementCount();
			n_assert_dbg(MaxLightCount > 0);

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

			if (GlobalLightCount) View.Globals.Apply();
		}

		// Setup IBL (ambient cubemaps)
		// Later we can implement local weight-blended parallax-corrected cubemaps selected by COI here
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

	// Sort render queue if requested

	switch (SortingType)
	{
		case Sort_FrontToBack:	RenderQueue.Sort<CRenderQueueCmp_FrontToBack>(); break;
		case Sort_Material:		RenderQueue.Sort<CRenderQueueCmp_Material>(); break;
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

	// Render the phase

	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.CameraPosition = CameraPos;
	Ctx.ViewProjection = View.GetCamera()->GetViewProjMatrix();
	if (EnableLighting)
	{
		Ctx.pLights = &View.GetLightCache();
		Ctx.pLightIndices = &View.LightIndices;
		Ctx.UsesGlobalLightBuffer = !!ConstGlobalLightBuffer;
	}
	else
	{
		Ctx.pLights = nullptr;
		Ctx.pLightIndices = nullptr;
		Ctx.UsesGlobalLightBuffer = false;
	}

	Render::CRenderQueueIterator ItCurr = RenderQueue.Begin();
	Render::CRenderQueueIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
		ItCurr = (*ItCurr)->pRenderer->Render(Ctx, RenderQueue, ItCurr);

	//!!!hide in a private method of CView!
	View.LightIndices.Clear(false);
	for (UPTR i = 0; i < RenderQueue.GetCount(); ++i)
		View.RenderNodePool.Destroy(RenderQueue[i]);
	RenderQueue.Clear(false);
	//???may store render queue in cache for other phases? or completely unreusable? some info like a distance to a camera may be shared

	// Unbind render target(s) etc
	//???allow each phase to declare all its RTs and clear unused ones by itself?
	//then unbind in the end of a CRenderPath::Render()

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, GfxMgr, PhaseName, Desc)) FAIL;

	EnableLighting = Desc.Get<bool>(CStrID("EnableLighting"), false);

	CString SortStr = Desc.Get<CString>(CStrID("Sort"), CString::Empty);
	SortStr.Trim();
	SortStr.ToLower();
	if (SortStr == "ftb" || SortStr == "fronttoback") SortingType = Sort_FrontToBack;
	else if (SortStr == "material") SortingType = Sort_Material;
	else SortingType = Sort_None;

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

	Data::CDataArray& RenderersDesc = *Desc.Get<Data::PDataArray>(CStrID("Renderers"));
	for (UPTR i = 0; i < RenderersDesc.GetCount(); ++i)
	{
		const Data::CParams& RendererDesc = *RenderersDesc[i].GetValue<Data::PParams>();

		// Renderer is useful only if it can render something
		const Data::CParam& PrmObject = RendererDesc.Get(CStrID("Objects"));
		if (!PrmObject.IsA<Data::PDataArray>() || PrmObject.GetValue<Data::PDataArray>()->IsEmpty()) FAIL;

		const Core::CRTTI* pRendererType = nullptr;
		const Data::CParam& PrmRenderer = RendererDesc.Get(CStrID("Renderer"));
		if (PrmRenderer.IsA<int>()) pRendererType = Core::CFactory::Instance().GetRTTI(static_cast<uint32_t>(PrmRenderer.GetValue<int>()));
		else if (PrmRenderer.IsA<CString>()) pRendererType = Core::CFactory::Instance().GetRTTI(PrmRenderer.GetValue<CString>());
		if (!pRendererType) FAIL;

		Render::PRenderer Renderer(static_cast<Render::IRenderer*>(pRendererType->CreateClassInstance()));
		if (!Renderer || !Renderer->Init(EnableLighting, RendererDesc)) FAIL;

		const auto& ObjTypes = *PrmObject.GetValue<Data::PDataArray>();
		for (const auto& ObjTypeData : ObjTypes)
		{
			const Core::CRTTI* pObjType = nullptr;
			if (ObjTypeData.IsA<int>())
				pObjType = Core::CFactory::Instance().GetRTTI(static_cast<uint32_t>(ObjTypeData.GetValue<int>()));
			else if (ObjTypeData.IsA<CString>())
				pObjType = Core::CFactory::Instance().GetRTTI(ObjTypeData.GetValue<CString>());
			if (!pObjType) FAIL;

			RenderersByObjectType.emplace(pObjType, Renderer.get());
		}

		Renderers.push_back(std::move(Renderer));
	}

	//!!!remember only IDs here, load effect in a View, as they reference a GPU!
	//anyway only one loaded copy of resource if possible now, so there can't be
	//two effect instances created with different GPUs
	n_assert_dbg(EffectOverrides.empty());
	Data::PParams EffectsDesc;
	if (Desc.TryGet(EffectsDesc, CStrID("Effects")))
	{
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
