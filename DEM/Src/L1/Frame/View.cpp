#include "View.h"

#include <Frame/NodeAttrCamera.h>
#include <Frame/RenderPath.h>
#include <Scene/SPS.h>
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
		if (!GlobalCBs.Contains(Const.BufferHandle))
		{
			Render::PConstantBuffer CB = GPU->CreateConstantBuffer(Const.BufferHandle, Render::Access_CPU_Write | Render::Access_GPU_Read);
			if (CB.IsNullPtr())
			{
				GlobalCBs.Clear();
				FAIL;
			}
			GlobalCBs.Add(Const.BufferHandle, CB);
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
