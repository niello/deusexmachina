#include "View.h"

#include <Frame/NodeAttrCamera.h>
#include <Frame/RenderPath.h>
#include <Scene/SPS.h>
#include <Render/GPUDriver.h>

namespace Frame
{

bool CView::SetCamera(CNodeAttrCamera* pNewCamera)
{
	if (pCamera == pNewCamera) OK;
	if (!pNewCamera)
	{
		pCamera = pNewCamera;
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
