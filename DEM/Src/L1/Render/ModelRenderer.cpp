#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/Mesh.h>
#include <Render/GPUDriver.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModelRenderer, 'MDLR', Render::IRenderer);

CModelRenderer::CModelRenderer()
{
	// Setup dynamic enumeration
	InputSet_Model = RegisterShaderInputSetID(CStrID("Model"));
	InputSet_ModelSkinned = RegisterShaderInputSetID(CStrID("ModelSkinned"));
	InputSet_ModelInstanced = RegisterShaderInputSetID(CStrID("ModelInstanced"));
}
//---------------------------------------------------------------------

//???return bool, if false, remove node from queue
//(array tail removal is very fast in CArray, can even delay removal in a case next RQ node will be added inplace)?
void CModelRenderer::PrepareNode(CRenderNode& Node)
{
	CModel* pModel = Node.pRenderable->As<CModel>();
	n_assert_dbg(pModel);

	//!!!can find once, outside the render loop! store somewhere in a persistent render node.
	//But this associates object and renderer, as other renderer may request other input set.
	Node.pMaterial = pModel->Material.GetUnsafe();
	Node.pTech = pModel->Material->GetEffect()->GetTechByInputSet(Node.pSkinPalette ? InputSet_ModelSkinned : InputSet_Model);
}
//---------------------------------------------------------------------

CArray<CRenderNode>::CIterator CModelRenderer::Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		CModel* pModel = ItCurr->pRenderable->As<CModel>();

		CVertexBuffer* pVB = pModel->Mesh->GetVertexBuffer().GetUnsafe();
		const CPrimitiveGroup* pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex/*, ItCurr->LOD*/);

		//HConst hWorld = pTech->GetParam(CStrID("WorldMatrix")); //???or find index and then reference by index?
		//!!!search in fallback materials if not found!

		// If tech supports instancing, try to collect more objects

		UPTR LightCount = 0;

		if (pVB && pGroup && ItCurr->pTech)
		{
			//!!!DBG TMP! move outside, not to redundantly reset!
			ItCurr->pMaterial->Apply(GPU);

			// Per-instance params
			//set tech params (feed shader according to an input set)
			//GPU.SetShaderConstant(TmpCB, hWorld, 0, ItCurr->Transform.m, sizeof(matrix44));
			//if (ItCurr->pSkinPalette) GPU.SetShaderConstant(TmpCB, hSkinPalette, 0, ItCurr->pSkinPalette, sizeof(matrix44) * ItCurr->BoneCount);

			GPU.SetVertexLayout(pVB->GetVertexLayout());
			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pModel->Mesh->GetIndexBuffer().GetUnsafe());

			const CPassList* pPasses = ItCurr->pTech->GetPasses(LightCount);
			if (pPasses)
			{
				for (UPTR i = 0; i < pPasses->GetCount(); ++i)
				{
					GPU.SetRenderState((*pPasses)[i]);
					GPU.Draw(*pGroup);
				}

				Sys::DbgOut("CModel rendered, tech '%s'\n", ItCurr->pTech->GetName().CStr());
			}
		}

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}