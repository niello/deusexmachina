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

CArray<CRenderNode>::CIterator CModelRenderer::Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		CModel* pModel = ItCurr->pRenderable->As<CModel>();
		n_assert_dbg(pModel);

		CVertexBuffer* pVB = pModel->Mesh->GetVertexBuffer().GetUnsafe();
		const CPrimitiveGroup* pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex/*, ItCurr->LOD*/);

		//???do outside a renderer, in a phase, before sorting, and store in renderable?
		//this requires a way to get material from a renderable. really need?
		//!!!can find once, outside the render loop! store somewhere, associates object and renderer
		const CTechnique* pTech = pModel->Material->GetEffect()->GetTechByInputSet(InputSet_Model);
		//HConst hWorld = pTech->GetParam(CStrID("WorldMatrix")); //???or find index and then reference by index?
		//!!!search in fallback materials if not found!

		// If tech supports instancing, try to collect more objects

		UPTR LightCount = 0;

		if (pVB && pGroup && pTech)
		{
			//!!!DBG TMP!
			pModel->Material->Apply(GPU);
			//set tech params (feed shader according to an input set)
			//GPU.SetShaderConstant(TmpCB, hWorld, 0, ItCurr->Transform.m, sizeof(matrix44));

			GPU.SetVertexLayout(pVB->GetVertexLayout());
			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pModel->Mesh->GetIndexBuffer().GetUnsafe());

			const CPassList* pPasses = pTech->GetPasses(LightCount);
			if (pPasses)
			{
				for (UPTR i = 0; i < pPasses->GetCount(); ++i)
				{
					GPU.SetRenderState((*pPasses)[i]);
					GPU.Draw(*pGroup);
				}

				Sys::DbgOut("CModel rendered, tech '%s'\n", pTech->GetName().CStr());
			}
		}

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}