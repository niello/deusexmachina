#include "View.h"

#include <Frame/NodeAttrCamera.h>
#include <Frame/NodeAttrAmbientLight.h>
#include <Frame/NodeAttrLight.h>
#include <Frame/RenderPath.h>
#include <Events/Subscription.h>
#include <Scene/SPS.h>
#include <Render/RenderTarget.h>
#include <Render/ShaderConstant.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/Sampler.h>
#include <Render/SamplerDesc.h>
#include <UI/UIContext.h>

namespace Frame
{
CView::CView() {}

CView::~CView()
{
	Globals.UnbindAndClear();
}
//---------------------------------------------------------------------

bool CView::SetRenderPath(CRenderPath* pNewRenderPath)
{
	// GPU must be valid in order to create global shader params and buffers
	if (GPU.IsNullPtr()) FAIL;

	if (RenderPath.GetUnsafe() == pNewRenderPath) OK;

	Globals.UnbindAndClear();
	Globals.SetGPU(GPU);

	if (!pNewRenderPath)
	{
		RTs.SetSize(0);
		RenderPath = NULL;
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

		const Render::HConstBuffer hCB = Const.Const->GetConstantBufferHandle();
		if (!Globals.IsBufferRegistered(hCB))
		{
			Render::PConstantBuffer CB = GPU->CreateConstantBuffer(hCB, Render::Access_CPU_Write | Render::Access_GPU_Read);
			if (CB.IsNullPtr())
			{
				Globals.UnbindAndClear();
				FAIL;
			}
			Globals.RegisterPermanentBuffer(hCB, *CB.GetUnsafe());
		}
	}

	// Create rlinear cube sampler for image-based lighting

	Render::CSamplerDesc SamplerDesc;
	SamplerDesc.SetDefaults();
	SamplerDesc.AddressU = Render::TexAddr_Clamp;
	SamplerDesc.AddressV = Render::TexAddr_Clamp;
	SamplerDesc.AddressW = Render::TexAddr_Clamp;
	SamplerDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	TrilinearCubeSampler = GPU->CreateSampler(SamplerDesc);

	RenderPath = pNewRenderPath;
	OK;
}
//---------------------------------------------------------------------

bool CView::SetCamera(CNodeAttrCamera* pNewCamera)
{
	if (pCamera == pNewCamera) OK;
	if (!pNewCamera)
	{
		pCamera = NULL;
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
	VisibilityCache.Clear();
	LightCache.Clear();
	EnvironmentCache.Clear();
	VisibilityCacheDirty = true;
	if (!GPU->BeginFrame() || !RenderPath->Render(*this)) FAIL;
	GPU->EndFrame();
	OK;
}
//---------------------------------------------------------------------

}
