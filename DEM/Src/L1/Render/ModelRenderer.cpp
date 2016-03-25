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

		const CPrimitiveGroup* pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex/*, ItCurr->LOD*/);

		//!!!can find once, outside the render loop! store somewhere, associates object and renderer
		const CTechnique* pTech = pModel->Material->GetEffect()->GetTechByInputSet(InputSet_Model);
		//!!!search in fallback materials if not found!

		// If tech supports instancing, try to collect more objects

		UPTR LightCount = 0;

		if (pGroup && pTech)
		{
			//set tech params

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