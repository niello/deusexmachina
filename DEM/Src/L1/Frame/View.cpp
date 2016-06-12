#include "View.h"

#include <Frame/NodeAttrCamera.h>
#include <Frame/RenderPath.h>
#include <Scene/SPS.h>
#include <Render/RenderTarget.h>
#include <Render/GPUDriver.h>

namespace Frame
{

bool CView::SetRenderPath(CRenderPath* pNewRenderPath)
{
	// GPU must be valid in order to create global constant buffers
	if (GPU.IsNullPtr()) FAIL;

	if (RenderPath.GetUnsafe() == pNewRenderPath) OK;
	if (!pNewRenderPath)
	{
		//!!!unbind CBs!
		GlobalCBs.Clear();
		RenderPath = NULL;
		OK;
	}

	const CFixedArray<Render::CEffectConstant>& GlobalConsts = pNewRenderPath->GetGlobalConstants();
	for (UPTR i = 0; i < GlobalConsts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Const = GlobalConsts[i];
		
		UPTR j = 0;
		for (; j < GlobalCBs.GetCount(); ++j)
			if (GlobalCBs[j].Handle == Const.BufferHandle) break;
		
		if (j == GlobalCBs.GetCount())
		{
			Render::PConstantBuffer CB = GPU->CreateConstantBuffer(Const.BufferHandle, Render::Access_CPU_Write | Render::Access_GPU_Read);
			if (CB.IsNullPtr())
			{
				GlobalCBs.Clear();
				FAIL;
			}
			CConstBufferRec* pRec = GlobalCBs.Add();
			pRec->Handle = Const.BufferHandle;
			pRec->Buffer = CB;
			pRec->ShaderTypes = 0; // Filled when const values are written, not to bind unnecessarily to all possibly requiring stages
		}
	}

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
		pSPS->QueryObjectsInsideFrustum(pCamera->GetViewProjMatrix(), VisibilityCache);
	else
		VisibilityCache.Clear();

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
	VisibilityCacheDirty = true;
	if (!GPU->BeginFrame() || !RenderPath->Render(*this)) FAIL;
	GPU->EndFrame();
	OK;
}
//---------------------------------------------------------------------

}
