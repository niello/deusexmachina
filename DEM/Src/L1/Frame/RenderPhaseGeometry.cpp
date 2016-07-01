#include "RenderPhaseGeometry.h"

#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrSkin.h>
#include <Scene/SceneNode.h>
#include <Render/Renderable.h>
#include <Render/Renderer.h>
#include <Render/RenderNode.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/SkinInfo.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <IO/PathUtils.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CRenderPhaseGeometry, 'PHGE', Frame::CRenderPhase);

struct CRenderQueueCmp_FrontToBack
{
	inline bool operator()(const Render::CRenderNode& a, const Render::CRenderNode& b) const
	{
		if (a.Order != b.Order) return a.Order < b.Order;
		return a.SqDistanceToCamera < b.SqDistanceToCamera;
	}
};
//---------------------------------------------------------------------

struct CRenderQueueCmp_Material
{
	inline bool operator()(const Render::CRenderNode& a, const Render::CRenderNode& b) const
	{
		if (a.Order != b.Order) return a.Order < b.Order;
		if (a.Order >= 40)
		{
			return a.SqDistanceToCamera > b.SqDistanceToCamera;
		}
		else
		{
			if (a.pTech != b.pTech) return a.pTech < b.pTech;
			if (a.pMaterial != b.pMaterial) return a.pMaterial < b.pMaterial;
			if (a.pMesh != b.pMesh) return a.pMesh < b.pMesh;
			if (a.pGroup != b.pGroup) return a.pGroup < b.pGroup;
			return a.SqDistanceToCamera < b.SqDistanceToCamera;
		}
	}
};
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Render(CView& View)
{
	if (!View.pSPS || !View.GetCamera()) OK;

	View.UpdateVisibilityCache();
	CArray<Scene::CNodeAttribute*>& VisibleObjects = View.GetVisibilityCache();

	if (!VisibleObjects.GetCount()) OK;

	const vector3& CameraPos = View.GetCamera()->GetPosition();
	const bool CalcScreenSize = View.RequiresScreenSize();

	CArray<Render::CRenderNode>& RenderQueue = View.RenderQueue;
	RenderQueue.Resize(VisibleObjects.GetCount());

	Render::CRenderNodeContext Context;
	Context.pEffectOverrides = EffectOverrides.GetCount() ? &EffectOverrides : NULL;

	for (CArray<Scene::CNodeAttribute*>::CIterator It = VisibleObjects.Begin(); It != VisibleObjects.End(); ++It)
	{
		Scene::CNodeAttribute* pAttr = *It;
		const Core::CRTTI* pAttrType = pAttr->GetRTTI();
		if (!pAttrType->IsDerivedFrom(Frame::CNodeAttrRenderable::RTTI)) continue; //!!!also need a light list! at the visibility cache construction?

		Frame::CNodeAttrRenderable* pAttrRenderable = (Frame::CNodeAttrRenderable*)pAttr;
		Render::IRenderable* pRenderable = pAttrRenderable->GetRenderable();

		IPTR Idx = Renderers.FindIndex(pRenderable->GetRTTI());
		if (Idx == INVALID_INDEX) continue;
		Render::IRenderer* pRenderer = Renderers.ValueAt(Idx);
		if (!pRenderer) continue;

		Render::CRenderNode* pNode = RenderQueue.Add();
		pNode->pRenderable = pRenderable;
		pNode->pRenderer = pRenderer;
		pNode->Transform = pAttr->GetNode()->GetWorldMatrix();

		Frame::CNodeAttrSkin* pSkinAttr = pAttr->GetNode()->FindFirstAttribute<Frame::CNodeAttrSkin>();
		if (pSkinAttr)
		{
			pNode->pSkinPalette = pSkinAttr->GetSkinPalette();
			pNode->BoneCount = pSkinAttr->GetSkinInfo()->GetBoneCount();
		}
		else pNode->pSkinPalette = NULL;

		CAABB GlobalAABB;
		if (pAttrRenderable->GetGlobalAABB(GlobalAABB, 0))
		{
			float ScreenSizeRel;
			if (CalcScreenSize)
			{
				NOT_IMPLEMENTED_MSG("SCREEN SIZE CALCULATION!");
				ScreenSizeRel = 0.f;
			}
			else ScreenSizeRel = 0.f;

			float SqDistanceToCamera = GlobalAABB.SqDistance(CameraPos);
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

		if (!pRenderer->PrepareNode(*pNode, Context))
		{
			RenderQueue.Remove(pNode);
			continue;
		}

		n_assert_dbg(pNode->pMaterial);
		Render::EEffectType Type = pNode->pMaterial->GetEffect()->GetType();
		switch (Type)
		{
			case Render::EffectType_Opaque:		pNode->Order = 10; break;
			case Render::EffectType_AlphaTest:	pNode->Order = 20; break;
			case Render::EffectType_Skybox:		pNode->Order = 30; break;
			case Render::EffectType_AlphaBlend:	pNode->Order = 40; break;
			default:							pNode->Order = 100; break;
		}
	}

	// Sort render queue if requested

	//???PERF: sort ptrs or indices into a render queue? CRenderNode structure may bee to big to be moved
	switch (SortingType)
	{
		case Sort_FrontToBack:	RenderQueue.Sort<CRenderQueueCmp_FrontToBack>(); break;
		case Sort_Material:		RenderQueue.Sort<CRenderQueueCmp_Material>(); break;
	}

	//???unbind unused or leave bound?
	for (UPTR i = 0; i < RenderTargetIndices.GetCount(); ++i)
		View.GPU->SetRenderTarget(i, View.RTs[RenderTargetIndices[i]].GetUnsafe());
	View.GPU->SetDepthStencilBuffer(DepthStencilIndex == INVALID_INDEX ? NULL : View.DSBuffers[DepthStencilIndex].GetUnsafe());

	CArray<Render::CRenderNode>::CIterator ItCurr = RenderQueue.Begin();
	CArray<Render::CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
		ItCurr = ItCurr->pRenderer->Render(*View.GPU, RenderQueue, ItCurr);

	RenderQueue.Clear(false);
	//???may store render queue in cache for other phases? or completely unreusable? some info like a distance to a camera may be shared

	// Unbind render target(s) etc
	//???allow each phase to declare all its RTs and clear unused ones by itself?
	//then unbind in the end of a CRenderPath::Render()

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGeometry::Init(CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(PhaseName, Desc)) FAIL;

	CString SortStr = Desc.Get<CString>(CStrID("Sort"), CString::Empty);
	SortStr.Trim();
	SortStr.ToLower();
	if (SortStr == "ftb" || SortStr == "fronttoback") SortingType = Sort_FrontToBack;
	else if (SortStr == "material") SortingType = Sort_Material;
	else SortingType = Sort_None;

	const Data::CData& RTValue = Desc.Get(CStrID("RenderTarget")).GetRawValue();
	if (RTValue.IsNull()) RenderTargetIndices.SetSize(0);
	else if (RTValue.IsA<Data::PDataArray>())
	{
		Data::PDataArray RTArray = RTValue.GetValue<Data::PDataArray>();
		RenderTargetIndices.SetSize(RTArray->GetCount());
		for (UPTR i = 0; i < RTArray->GetCount(); ++i)
			RenderTargetIndices[i] = (I32)RTArray->Get<int>(i);
	}
	else if (RTValue.IsA<int>())
	{
		RenderTargetIndices.SetSize(1);
		RenderTargetIndices[0] = (I32)RTValue.GetValue<int>();
	}
	else FAIL;

	const Data::CData& DSValue = Desc.Get(CStrID("DepthStencilBuffer")).GetRawValue();
	if (DSValue.IsNull()) DepthStencilIndex = INVALID_INDEX;
	else if (DSValue.IsA<int>())
	{
		DepthStencilIndex = (I32)DSValue.GetValue<int>();
	}
	else FAIL;

	Data::CDataArray& RenderersDesc = *Desc.Get<Data::PDataArray>(CStrID("Renderers"));
	for (UPTR i = 0; i < RenderersDesc.GetCount(); ++i)
	{
		Data::CParams& RendererDesc = *RenderersDesc[i].GetValue<Data::PParams>();

		const Core::CRTTI* pObjType = NULL;
		const Data::CParam& PrmObject = RendererDesc.Get(CStrID("Object"));
		if (PrmObject.IsA<int>()) pObjType = Factory->GetRTTI(Data::CFourCC((I32)PrmObject.GetValue<int>()));
		else if (PrmObject.IsA<CString>()) pObjType = Factory->GetRTTI(PrmObject.GetValue<CString>());
		if (!pObjType) FAIL;

		const Core::CRTTI* pRendererType = NULL;
		const Data::CParam& PrmRenderer = RendererDesc.Get(CStrID("Renderer"));
		if (PrmRenderer.IsA<int>()) pRendererType = Factory->GetRTTI(Data::CFourCC((I32)PrmRenderer.GetValue<int>()));
		else if (PrmRenderer.IsA<CString>()) pRendererType = Factory->GetRTTI(PrmRenderer.GetValue<CString>());
		if (!pRendererType) FAIL;

		Render::IRenderer* pRenderer = NULL;
		for (UPTR j = 0; j < Renderers.GetCount(); ++j)
			if (Renderers.ValueAt(j)->GetRTTI() == pRendererType)
			{
				pRenderer = Renderers.ValueAt(j);
				break;
			}
		if (!pRenderer) pRenderer = (Render::IRenderer*)pRendererType->CreateClassInstance();
		if (pObjType && pRenderer) Renderers.Add(pObjType, pRenderer);
	}

	//!!!remember only IDs here, load effect in a View, as they reference a GPU!
	//anyway only one loaded copy of resource if possible now, so there can't be
	//two effect instances created with different GPUs
	n_assert_dbg(!EffectOverrides.GetCount());
	Data::PParams EffectsDesc;
	if (Desc.Get(EffectsDesc, CStrID("Effects")))
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

			Render::PEffect Effect;
			if (!Prm.GetRawValue().IsNull())
			{
				CString RsrcURI("Effects:");
				RsrcURI += Prm.GetValue<CStrID>().CStr();
				RsrcURI += ".eff"; //???replace ID by full URI on export?

				Resources::PResource Rsrc = ResourceMgr->RegisterResource(RsrcURI.CStr());
				if (!Rsrc->IsLoaded())
				{
					Resources::PResourceLoader Loader = Rsrc->GetLoader();
					if (Loader.IsNullPtr())
					{
						Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CEffect>(PathUtils::GetExtension(RsrcURI.CStr()));
						if (Loader.IsNullPtr()) FAIL;
					}
					ResourceMgr->LoadResourceSync(*Rsrc, *Loader);
					if (!Rsrc->IsLoaded()) FAIL;
				}
				Effect = Rsrc->GetObject<Render::CEffect>();
			}

			EffectOverrides.Add(EffectType, Effect);
		}
	}

	OK;
}
//---------------------------------------------------------------------

}
