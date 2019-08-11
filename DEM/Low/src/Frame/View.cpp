#include "View.h"

#include <Frame/NodeAttrCamera.h>
#include <Frame/NodeAttrAmbientLight.h>
#include <Frame/NodeAttrLight.h>
#include <Frame/RenderPath.h>
#include <Scene/SPS.h>
#include <Render/RenderTarget.h>
#include <Render/ShaderConstant.h>
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

CView::CView(CRenderPath& RenderPath, Render::CGPUDriver& GPU, int SwapChainID)
	: _GPU(&GPU)
	, _SwapChainID(SwapChainID)
{
	SetRenderPath(&RenderPath);

	// If our view is attached to the swap chain, set the back buffer as its first RT
	if (SwapChainID >= 0 && RTs.GetCount())
		RTs[0] = GPU.GetSwapChainRenderTarget(SwapChainID);
}
//---------------------------------------------------------------------

CView::~CView()
{
	Globals.UnbindAndClear();
}
//---------------------------------------------------------------------

bool CView::CreateUIContext()
{
	Render::CRenderTarget* pRT = RTs[0].Get();
	if (!UI::CUIServer::HasInstance() || !pRT) FAIL;

	UI::CUIContextSettings UICtxSettings;
	UICtxSettings.HostWindow = GetTargetWindow();
	UICtxSettings.Width = static_cast<float>(pRT->GetDesc().Width);
	UICtxSettings.Height = static_cast<float>(pRT->GetDesc().Height);
	UIContext = UISrv->CreateContext(UICtxSettings);

	return (UIContext != nullptr);
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
			if (RTs[0].IsNullPtr()) return 0;
			const Render::CRenderTargetDesc& RTDesc = RTs[0]->GetDesc();
			float ScreenSpaceOccupiedAbs = SqDistanceToCamera * RTDesc.Width * RTDesc.Height; //!!!may cache float WMulH, on each RT set!
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
			if (RTs[0].IsNullPtr()) return 0;
			const Render::CRenderTargetDesc& RTDesc = RTs[0]->GetDesc();
			float ScreenSpaceOccupiedAbs = SqDistanceToCamera * RTDesc.Width * RTDesc.Height; //!!!may cache float WMulH, on each RT set!
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
	return _SwapChainID >= 0 && _GPU && _GPU->Present(_SwapChainID);
}
//---------------------------------------------------------------------

bool CView::SetRenderPath(CRenderPath* pNewRenderPath)
{
	// GPU must be valid in order to create global shader params and buffers
	if (!_GPU) FAIL;

	if (_RenderPath.Get() == pNewRenderPath) OK;

	Globals.UnbindAndClear();
	Globals.SetGPU(_GPU);

	if (!pNewRenderPath)
	{
		RTs.SetSize(0);
		_RenderPath = nullptr;
		OK;
	}

	// Allocate RT and DS slots
	//!!!may fill with default RTs and DSs if descs are provided in RP!

	RTs.SetSize(pNewRenderPath->GetRenderTargetCount());
	DSBuffers.SetSize(pNewRenderPath->GetDepthStencilBufferCount());

	// Allocate storage for global shader params

	const CFixedArray<Render::CEffectConstant>& GlobalConsts = pNewRenderPath->GetGlobalConstants();
	for (UPTR i = 0; i < GlobalConsts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Const = GlobalConsts[i];

		const Render::HConstantBuffer hCB = Const.Const->GetConstantBufferHandle();
		if (!Globals.IsBufferRegistered(hCB))
		{
			Render::PConstantBuffer CB = _GPU->CreateConstantBuffer(hCB, Render::Access_CPU_Write | Render::Access_GPU_Read);
			if (CB.IsNullPtr())
			{
				Globals.UnbindAndClear();
				FAIL;
			}
			Globals.RegisterPermanentBuffer(hCB, *CB.Get());
		}
	}

	// Create linear cube sampler for image-based lighting
	//???FIXME: declarative in RP?

	Render::CSamplerDesc SamplerDesc;
	SamplerDesc.SetDefaults();
	SamplerDesc.AddressU = Render::TexAddr_Clamp;
	SamplerDesc.AddressV = Render::TexAddr_Clamp;
	SamplerDesc.AddressW = Render::TexAddr_Clamp;
	SamplerDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	TrilinearCubeSampler = _GPU->CreateSampler(SamplerDesc);

	_RenderPath = pNewRenderPath;
	OK;
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

DEM::Sys::COSWindow* CView::GetTargetWindow() const
{
	return _GPU ? _GPU->GetSwapChainWindow(_SwapChainID) : nullptr;
}
//---------------------------------------------------------------------

Render::PDisplayDriver CView::GetTargetDisplay() const
{
	return _GPU ? _GPU->GetSwapChainDisplay(_SwapChainID) : nullptr;
}
//---------------------------------------------------------------------

bool CView::IsFullscreen() const
{
	return _SwapChainID >= 0 && _GPU && _GPU->IsFullscreen(_SwapChainID);
}
//---------------------------------------------------------------------

}
