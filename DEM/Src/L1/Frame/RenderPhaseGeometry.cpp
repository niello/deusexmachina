#include "RenderPhaseGeometry.h"

#include <Frame/View.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrSkin.h>
#include <Scene/SceneNode.h>
#include <Render/Renderable.h>
#include <Render/Renderer.h>
#include <Render/RenderNode.h>
#include <Render/GPUDriver.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CRenderPhaseGeometry, 'PHGE', Frame::CRenderPhase);

bool CRenderPhaseGeometry::Render(CView& View)
{
	if (!View.pSPS || !View.GetCamera()) OK;

	View.UpdateVisibilityCache();
	CArray<Scene::CNodeAttribute*>& VisibleObjects = View.GetVisibilityCache();

	if (!VisibleObjects.GetCount()) OK;

	//!!!can query correct squared distance to camera and/or screen size from an SPS along with object pointers!

	CArray<Render::CRenderNode>& RenderQueue = View.RenderQueue;
	RenderQueue.Resize(VisibleObjects.GetCount());

	for (CArray<Scene::CNodeAttribute*>::CIterator It = VisibleObjects.Begin(); It != VisibleObjects.End(); ++It)
	{
		Scene::CNodeAttribute* pAttr = *It;
		const Core::CRTTI* pAttrType = pAttr->GetRTTI();
		if (!pAttrType->IsDerivedFrom(Frame::CNodeAttrRenderable::RTTI)) continue; //!!!also need a light list! at the visibility cache construction?

		Render::IRenderable* pRenderable = ((Frame::CNodeAttrRenderable*)pAttr)->GetRenderable();

		IPTR Idx = Renderers.FindIndex(pRenderable->GetRTTI());
		if (Idx == INVALID_INDEX) continue;
		Render::IRenderer* pRenderer = Renderers.ValueAt(Idx);
		if (!pRenderer) continue;

		Render::CRenderNode* pNode = RenderQueue.Add();
		pNode->pRenderable = pRenderable;
		pNode->pRenderer = pRenderer;
		pNode->Transform = pAttr->GetNode()->GetWorldMatrix();
		//pNode->LOD = 0; //!!!determine based on sq camera or screen size! in renderers? sphere screen size may be reduced to 2 points = len of square
		//!!!if LOD in renderers, may store sq dist to camera and/or screen size in a render node, and use them to determine LOD!
		Frame::CNodeAttrSkin* pSkinAttr = pAttr->GetNode()->FindFirstAttribute<Frame::CNodeAttrSkin>();
		if (pSkinAttr)
		{
			//!!!add skin palette to a render node!
		}
	}

	// Sort render queue if necessary
	//???or sort in renderers? or build tree and sort each branch as required
	//may write complex sorter which takes into account alpha

	for (UPTR i = 0; i < View.RTs.GetCount(); ++i)
		View.GPU->SetRenderTarget(i, View.RTs[i]);
	View.GPU->SetDepthStencilBuffer(View.DSBuffer.GetUnsafe());

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

	OK;
}
//---------------------------------------------------------------------

//bool CPassGeometry::Init(CStrID PassName, const Data::CParams& Desc, const CDict<CStrID, PRenderTarget>& RenderTargets)
//{
//	if (!CRenderPhase::Init(PassName, Desc, RenderTargets)) FAIL;
//
//	Data::CDataArray& Batches = *Desc.Get<Data::PDataArray>(CStrID("Batches"));
//	for (int i = 0; i < Batches.GetCount(); ++i)
//	{
//		Data::CParams& BatchDesc = *(Data::PParams)Batches[i];
//		PRenderer& Renderer = BatchRenderers.At(i);
//		Renderer = (IRenderer*)Factory->Create(BatchDesc.Get<CString>(CStrID("Renderer")));
//		if (!Renderer->Init(BatchDesc)) FAIL;
//	}
//
//	OK;
//}
////---------------------------------------------------------------------
//
//void CPassGeometry::Render(const CArray<IRenderable*>* pObjects, const CArray<CLight*>* pLights)
//{
//	if (Shader.IsValid())
//	{
//		for (int i = 0; i < ShaderVars.GetCount(); ++i)
//			ShaderVars.ValueAt(i).Apply(*Shader.GetUnsafe());
//		n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!
//		Shader->BeginPass(0);
//	}
//
//	for (int i = 0; i < CRenderServer::MaxRenderTargetCount; ++i)
//		//if (RT[i].IsValid()) // Now sets NULL RTs too to clear unused RTs from prev. passes. See alternative below.
//			RenderSrv->SetRenderTarget(i, RT[i].GetUnsafe());
//
//	RenderSrv->Clear(ClearFlags, ClearColor, ClearDepth, ClearStencil);
//
//	// N3: set pixel size and half pixel size shared shader vars //???why not committed in N3?
//	//!!!soft particles use it to sample depth!
//
//	for (int i = 0; i < BatchRenderers.GetCount(); ++i)
//	{
//		IRenderer* pRenderer = BatchRenderers[i];
//		if (pLights) pRenderer->AddLights(*pLights);
//		if (pObjects) pRenderer->AddRenderObjects(*pObjects);
//		pRenderer->Render();
//	}
//
//	for (int i = 0; i < CRenderServer::MaxRenderTargetCount; ++i)
//		if (RT[i].IsValid()) //???break on first invalid? RTs must be set in order
//		{
//			RT[i]->Resolve();
//			// N3: if (i > 0) RenderSrv->SetRenderTarget(i, NULL);
//		}
//
//	if (Shader.IsValid())
//	{
//		Shader->EndPass();
//		Shader->End();
//	}
//}
////---------------------------------------------------------------------

}
