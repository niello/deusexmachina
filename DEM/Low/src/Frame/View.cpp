#include "View.h"
#include <Frame/CameraAttribute.h>
#include <Frame/AmbientLightAttribute.h>
#include <Frame/LightAttribute.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/RenderPath.h>
#include <Frame/GraphicsResourceManager.h>
#include <Scene/SPS.h>
#include <Render/Renderable.h>
#include <Render/RenderTarget.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>
#include <Render/DisplayDriver.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/Sampler.h>
#include <Render/SamplerDesc.h>
#include <Debug/DebugDraw.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <System/OSWindow.h>
#include <System/SystemEvents.h>

namespace Frame
{

CView::CView(CGraphicsResourceManager& GraphicsMgr, CStrID RenderPathID, int SwapChainID, CStrID SwapChainRTID)
	: _GraphicsMgr(&GraphicsMgr)
	, _SwapChainID(SwapChainID)
{
	// Obtain the render path resource

	_RenderPath = GraphicsMgr.GetRenderPath(RenderPathID);
	if (!_RenderPath)
	{
		::Sys::Error("CView() > no render path with ID " + RenderPathID);
		return;
	}

	Globals = Render::CShaderParamStorage(_RenderPath->GetGlobalParamTable(), *GraphicsMgr.GetGPU(), true);
		
	// Allocate storage for global shader params

	auto& GlobalParams = _RenderPath->GetGlobalParamTable();
	for (const auto& Const : GlobalParams.GetConstants())
		Globals.CreatePermanentConstantBuffer(Const.GetConstantBufferIndex(), Render::Access_CPU_Write | Render::Access_GPU_Read);

	// Create linear cube sampler for image-based lighting
	//???FIXME: declarative in RP? as material defaults in the effect!

	Render::CSamplerDesc SamplerDesc;
	SamplerDesc.SetDefaults();
	SamplerDesc.AddressU = Render::TexAddr_Clamp;
	SamplerDesc.AddressV = Render::TexAddr_Clamp;
	SamplerDesc.AddressW = Render::TexAddr_Clamp;
	SamplerDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	TrilinearCubeSampler = GraphicsMgr.GetGPU()->CreateSampler(SamplerDesc);

	//!!!may fill with default RTs and DSs if descs are provided in RP!

	// If our view is attached to the swap chain, set the back buffer as its first RT
	if (SwapChainID >= 0 && SwapChainRTID)
	{
		SetRenderTarget(SwapChainRTID, GraphicsMgr.GetGPU()->GetSwapChainRenderTarget(SwapChainID));
		if (auto pOSWindow = GetTargetWindow())
			DISP_SUBSCRIBE_NEVENT(pOSWindow, OSWindowResized, CView, OnOSWindowResized);
	}
}
//---------------------------------------------------------------------

CView::~CView() = default;
//---------------------------------------------------------------------

// Pass empty RenderTargetID to use swap chain render target
bool CView::CreateUIContext(CStrID RenderTargetID)
{
	auto* pUI = _GraphicsMgr->GetUI();
	if (!pUI || RTs.empty()) FAIL;

	const Render::CRenderTarget* pRT = nullptr;
	const Render::CRenderTarget* pSwapChainRT = (_SwapChainID >= 0) ? _GraphicsMgr->GetGPU()->GetSwapChainRenderTarget(_SwapChainID) : nullptr;
	if (RenderTargetID)
	{
		auto It = RTs.find(RenderTargetID);
		if (It == RTs.cend() || !It->second) FAIL;
		pRT = It->second;
	}
	else if (pSwapChainRT)
	{
		pRT = pSwapChainRT;
	}
	else FAIL;

	_UIContext = pUI->CreateContext(
		static_cast<float>(pRT->GetDesc().Width),
		static_cast<float>(pRT->GetDesc().Height),
		(pRT == pSwapChainRT) ? GetTargetWindow() : nullptr);

	return _UIContext.IsValidPtr();
}
//---------------------------------------------------------------------

bool CView::CreateDebugDrawer()
{
	_DebugDraw.reset(n_new(Debug::CDebugDraw(*_GraphicsMgr)));
	return !!_DebugDraw;
}
//---------------------------------------------------------------------

//!!!IRenderable children may now serve as parts of the rendering cache / render nodes!
Render::IRenderable* CView::GetRenderObject(const CRenderableAttribute& Attr)
{
	auto It = _RenderObjects.find(&Attr);
	if (It == _RenderObjects.cend())
	{
		// Different views on the same GPU will create different renderables,
		// which is not necessary. Render object cache might be stored in a
		// graphics manager, but it seems wrong for now. Anyway, GPU resources
		// are shared between these objects and views.
		auto Renderable = Attr.CreateRenderable(*_GraphicsMgr);
		if (!Renderable) return nullptr;
		It = _RenderObjects.emplace(&Attr, std::move(Renderable)).first;
	}

	return It->second.get();
}
//---------------------------------------------------------------------

void CView::UpdateVisibilityCache()
{
	if (!VisibilityCacheDirty) return;

	if (pSPS && pCamera)
	{
		pSPS->QueryObjectsInsideFrustum(pCamera->GetViewProjMatrix(), VisibilityCache);

		for (UPTR i = 0; i < VisibilityCache.GetCount(); /**/)
		{
			Scene::CNodeAttribute* pAttr = VisibilityCache[i];
			if (pAttr->IsA<CLightAttribute>())
			{
				Render::CLightRecord& Rec = *LightCache.Add();
				Rec.pLight = &((CLightAttribute*)pAttr)->GetLight();
				Rec.Transform = pAttr->GetNode()->GetWorldMatrix();
				Rec.UseCount = 0;
				Rec.GPULightIndex = INVALID_INDEX;

				VisibilityCache.RemoveAt(i);
			}
			else if (pAttr->IsA<CAmbientLightAttribute>())
			{
				EnvironmentCache.Add(pAttr->As<CAmbientLightAttribute>());
				VisibilityCache.RemoveAt(i);
			}
			else ++i;
		}
	}
	else
	{
		VisibilityCache.Clear();
		LightCache.Clear();
		EnvironmentCache.Clear();
	}

	VisibilityCacheDirty = false;
}
//---------------------------------------------------------------------

UPTR CView::GetMeshLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const
{
	switch (MeshLODType)
	{
		case LOD_None:
		{
			return 0;
		}
		case LOD_Distance:
		{
			for (UPTR i = 0; i < MeshLODScale.GetCount(); ++i)
				if (SqDistanceToCamera < MeshLODScale[i]) return i;
			return MeshLODScale.GetCount();
		}
		case LOD_ScreenSizeRelative:
		{
			for (UPTR i = 0; i < MeshLODScale.GetCount(); ++i)
				if (ScreenSpaceOccupiedRel > MeshLODScale[i]) return i;
			return MeshLODScale.GetCount();
		}
		case LOD_ScreenSizeAbsolute:
		{
			// FIXME: what render target to use? Not always the first one definitely!
			if (!RTs.begin()->second) return 0;
			const Render::CRenderTargetDesc& RTDesc = RTs.begin()->second->GetDesc();
			const float ScreenSpaceOccupiedAbs = SqDistanceToCamera * RTDesc.Width * RTDesc.Height;
			for (UPTR i = 0; i < MeshLODScale.GetCount(); ++i)
				if (ScreenSpaceOccupiedAbs > MeshLODScale[i]) return i;
			return MeshLODScale.GetCount();
		}
	}

	return 0;
}
//---------------------------------------------------------------------

UPTR CView::GetMaterialLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const
{
	switch (MaterialLODType)
	{
		case LOD_None:
		{
			return 0;
		}
		case LOD_Distance:
		{
			for (UPTR i = 0; i < MaterialLODScale.GetCount(); ++i)
				if (SqDistanceToCamera < MaterialLODScale[i]) return i;
			return MaterialLODScale.GetCount();
		}
		case LOD_ScreenSizeRelative:
		{
			for (UPTR i = 0; i < MaterialLODScale.GetCount(); ++i)
				if (ScreenSpaceOccupiedRel > MaterialLODScale[i]) return i;
			return MaterialLODScale.GetCount();
		}
		case LOD_ScreenSizeAbsolute:
		{
			// FIXME: what render target to use? Not always the first one definitely!
			if (!RTs.begin()->second) return 0;
			const Render::CRenderTargetDesc& RTDesc = RTs.begin()->second->GetDesc();
			const float ScreenSpaceOccupiedAbs = SqDistanceToCamera * RTDesc.Width * RTDesc.Height;
			for (UPTR i = 0; i < MaterialLODScale.GetCount(); ++i)
				if (ScreenSpaceOccupiedAbs > MaterialLODScale[i]) return i;
			return MaterialLODScale.GetCount();
		}
	}

	return 0;
}
//---------------------------------------------------------------------

void CView::Update(float dt)
{
	if (_GraphicsMgr) _GraphicsMgr->Update(dt);
	if (_UIContext) _UIContext->Update(dt);
}
//---------------------------------------------------------------------

bool CView::Render()
{
	if (!_RenderPath) FAIL;

	VisibilityCache.Clear();
	LightCache.Clear();
	EnvironmentCache.Clear();
	VisibilityCacheDirty = true;

	return _RenderPath->Render(*this);
}
//---------------------------------------------------------------------

// NB: it is recommended to call Present as late as possible after the last
// rendering command. Call it just before rendering the next frame.
// This method results in a no-op for intermediate RTs.
bool CView::Present() const
{
	if (_SwapChainID < 0) FAIL;

	auto GPU = GetGPU();
	return GPU && GPU->Present(_SwapChainID);
}
//---------------------------------------------------------------------

// FIXME: what if swap chain target is replaced and then window resizing happens?
// Must unsubscribe or resubscribe, dependent on what a new RT is.
bool CView::SetRenderTarget(CStrID ID, Render::PRenderTarget RT)
{
	if (!_RenderPath || !_RenderPath->HasRenderTarget(ID)) FAIL;

	if (RT) RTs[ID] = RT;
	else RTs.erase(ID);
	OK;
}
//---------------------------------------------------------------------

Render::CRenderTarget* CView::GetRenderTarget(CStrID ID) const
{
	auto It = RTs.find(ID);
	return (It != RTs.cend()) ? It->second.Get() : nullptr;
}
//---------------------------------------------------------------------

bool CView::SetDepthStencilBuffer(CStrID ID, Render::PDepthStencilBuffer DS)
{
	if (!_RenderPath || !_RenderPath->HasDepthStencilBuffer(ID)) FAIL;

	if (DS) DSBuffers[ID] = DS;
	else DSBuffers.erase(ID);
	OK;
}
//---------------------------------------------------------------------

Render::CDepthStencilBuffer* CView::GetDepthStencilBuffer(CStrID ID) const
{
	auto It = DSBuffers.find(ID);
	return (It != DSBuffers.cend()) ? It->second.Get() : nullptr;
}
//---------------------------------------------------------------------

bool CView::SetCamera(CCameraAttribute* pNewCamera)
{
	if (pCamera == pNewCamera) OK;
	if (!pNewCamera)
	{
		pCamera = nullptr;
		OK;
	}

	Scene::CSceneNode* pNode = pNewCamera->GetNode();
	if (!pNode) FAIL;

	pCamera = pNewCamera;
	VisibilityCacheDirty = true;
	OK;
}
//---------------------------------------------------------------------

CGraphicsResourceManager* CView::GetGraphicsManager() const
{
	return _GraphicsMgr;
}
//---------------------------------------------------------------------

Render::CGPUDriver* CView::GetGPU() const
{
	return _GraphicsMgr ? _GraphicsMgr->GetGPU() : nullptr;
}
//---------------------------------------------------------------------

DEM::Sys::COSWindow* CView::GetTargetWindow() const
{
	auto GPU = GetGPU();
	return GPU ? GPU->GetSwapChainWindow(_SwapChainID) : nullptr;
}
//---------------------------------------------------------------------

Render::PDisplayDriver CView::GetTargetDisplay() const
{
	auto GPU = GetGPU();
	return GPU ? GPU->GetSwapChainDisplay(_SwapChainID) : nullptr;
}
//---------------------------------------------------------------------

bool CView::IsFullscreen() const
{
	if (_SwapChainID < 0) FAIL;

	auto GPU = GetGPU();
	return GPU && GPU->IsFullscreen(_SwapChainID);
}
//---------------------------------------------------------------------

// TODO: fullscreen handling here too?
bool CView::OnOSWindowResized(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const auto& Ev = static_cast<const Event::OSWindowResized&>(Event);
	auto GPU = GetGPU();

	if (!GPU || Ev.ManualResizingInProgress) FAIL;

	// May not be fired in fullscreen mode by design. If happened, related code may need rewriting.
	n_assert_dbg(!GPU->IsFullscreen(_SwapChainID));

	GPU->SetDepthStencilBuffer(nullptr);

	GPU->ResizeSwapChain(_SwapChainID, Ev.NewWidth, Ev.NewHeight);

	//???really need to resize ALL DS buffers? May need some flag declared in a render path or CView::SetDepthStencilAutosized()
	//or add into render path an ability to specify RT/DS size relative to swap chain or some other RT / DS.
	for (auto& [ID, DS] : DSBuffers)
	{
		if (!DS) continue;

		auto Desc = DS->GetDesc();
		Desc.Width = Ev.NewWidth;
		Desc.Height = Ev.NewHeight;

		DS = GPU->CreateDepthStencilBuffer(Desc);
	}

	if (pCamera)
	{
		pCamera->SetWidth(Ev.NewWidth);
		pCamera->SetHeight(Ev.NewHeight);
	}

	OK;
}
//---------------------------------------------------------------------

}
