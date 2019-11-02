#include "View.h"
#include <Frame/NodeAttrCamera.h>
#include <Frame/NodeAttrAmbientLight.h>
#include <Frame/NodeAttrLight.h>
#include <Frame/RenderPath.h>
#include <Frame/GraphicsResourceManager.h>
#include <Scene/SPS.h>
#include <Render/RenderTarget.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>
#include <Render/DisplayDriver.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/Sampler.h>
#include <Render/SamplerDesc.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <System/OSWindow.h>

namespace Frame
{
CView::CView() {}

CView::CView(CRenderPath& RenderPath, CGraphicsResourceManager& GraphicsMgr, int SwapChainID, CStrID SwapChainRTID)
	: _GraphicsMgr(&GraphicsMgr)
	, _SwapChainID(SwapChainID)
{
	SetRenderPath(&RenderPath);

	// If our view is attached to the swap chain, set the back buffer as its first RT
	if (SwapChainID >= 0 && SwapChainRTID)
		SetRenderTarget(SwapChainRTID, GraphicsMgr.GetGPU()->GetSwapChainRenderTarget(SwapChainID));
}
//---------------------------------------------------------------------

CView::~CView()
{
	//???must be in a CShaderParamStorage destructor? or control unbinding manually? or bool flag in constructor?
	Globals.UnbindAndClear();
}
//---------------------------------------------------------------------

bool CView::CreateUIContext(CStrID RenderTargetID)
{
	if (!UI::CUIServer::HasInstance() || RTs.empty()) FAIL;

	// Get render target for resolution
	Render::CRenderTarget* pRT = nullptr;
	if (RenderTargetID)
	{
		auto It = RTs.find(RenderTargetID);
		if (It == RTs.cend()) FAIL;
		pRT = It->second;
	}
	else if (_GraphicsMgr && _SwapChainID >= 0)
	{
		pRT = _GraphicsMgr->GetGPU()->GetSwapChainRenderTarget(_SwapChainID);
	}

	if (!pRT) FAIL;

	UI::CUIContextSettings UICtxSettings;
	UICtxSettings.HostWindow = GetTargetWindow();
	UICtxSettings.Width = static_cast<float>(pRT->GetDesc().Width);
	UICtxSettings.Height = static_cast<float>(pRT->GetDesc().Height);
	UIContext = UISrv->CreateContext(UICtxSettings);

	return UIContext.IsValidPtr();
}
//---------------------------------------------------------------------

void CView::UpdateVisibilityCache()
{
	if (!VisibilityCacheDirty) return;

	if (pSPS && pCamera)
	{
		pSPS->QueryObjectsInsideFrustum(pCamera->GetViewProjMatrix(), VisibilityCache);

		for (UPTR i = 0; i < VisibilityCache.GetCount();)
		{
			Scene::CNodeAttribute* pAttr = VisibilityCache[i];
			if (pAttr->IsA<CNodeAttrLight>())
			{
				Render::CLightRecord& Rec = *LightCache.Add();
				Rec.pLight = &((CNodeAttrLight*)pAttr)->GetLight();
				Rec.Transform = pAttr->GetNode()->GetWorldMatrix();
				Rec.UseCount = 0;
				Rec.GPULightIndex = INVALID_INDEX;

				VisibilityCache.RemoveAt(i);
			}
			else if (pAttr->IsA<CNodeAttrAmbientLight>())
			{
				EnvironmentCache.Add(pAttr->As<CNodeAttrAmbientLight>());
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

bool CView::SetRenderPath(CRenderPath* pNewRenderPath)
{
	// GPU must be valid in order to create global shader params and buffers
	if (!_GraphicsMgr) FAIL;

	if (_RenderPath.Get() == pNewRenderPath) OK;

	auto GPU = GetGPU();

	//???must be in a CShaderParamStorage destructor? or control unbinding manually? or bool flag in constructor?
	Globals.UnbindAndClear();

	auto& GlobalParams = pNewRenderPath->GetGlobalParamTable();
	Globals = Render::CShaderParamStorage(GlobalParams, *GPU);

	RTs.clear();
	DSBuffers.clear();

	if (!pNewRenderPath)
	{
		_RenderPath = nullptr;
		OK;
	}

	//!!!may fill with default RTs and DSs if descs are provided in RP!

	// Allocate storage for global shader params

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
	TrilinearCubeSampler = GPU->CreateSampler(SamplerDesc);

	_RenderPath = pNewRenderPath;
	OK;
}
//---------------------------------------------------------------------

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

bool CView::SetCamera(CNodeAttrCamera* pNewCamera)
{
	if (pCamera == pNewCamera) OK;
	if (!pNewCamera)
	{
		pCamera = nullptr;
		OK;
	}

	Scene::CSceneNode* pNode = pNewCamera->GetNode();
	if (!pNode) FAIL;

	//!!!if scene node specified, test here if camera belongs to the same scene!

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

}
